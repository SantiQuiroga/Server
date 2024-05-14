#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 30

const char *server_info = "Server: HTTP Server/1.0\r\n\r\n";
int server_fd;

char *execute_python_script(char *script_path) {
    FILE *python_output;
    char python_command[BUFFER_SIZE];
    static char python_buffer[BUFFER_SIZE * BUFFER_SIZE];
    int status;
    pid_t pid;

    snprintf(python_command, sizeof(python_command), "python3 %s", script_path);
    python_output = popen(python_command, "r");
    if (python_output == NULL) {
        perror("Failed to run python script.");
        return NULL;
    }

    char line[BUFFER_SIZE];
    python_buffer[0] = '\0';
    while (fgets(line, sizeof(line), python_output) != NULL) {
        strncat(python_buffer, line, sizeof(python_buffer) - strlen(python_buffer) - 1);
    }

    pid = waitpid(-1, &status, WNOHANG);
    if (pid == 0) {
        if (pclose(python_output) == -1) {
            perror("Failed to close python script process.");
        }
    }

    return python_buffer;
}

void sigchld_handler() {
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

void setup_sigchld_handler() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("Failed to set up SIGCHLD handler.");
        exit(EXIT_FAILURE);
    }
}

void exit_server() {
    printf("\nShutting down server...\n");
    close(server_fd);
    exit(0);
}

void send_server_info(int client_socket, const char *status_code, const char *content_type) {
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), "HTTP/1.0 %s\r\nContent-Type: %s\r\n\r\n", status_code, content_type);
    send(client_socket, response, strlen(response), 0);
}

const char *get_content_type(char *path) {
    char *extension = strrchr(path, '.');

    if (extension == NULL) {
        return "application/octet-stream";
    } else if (strcmp(extension, ".html") == 0 || strcmp(extension, ".htm") == 0) {
        return "text/html";
    } else if (strcmp(extension, ".jpg") == 0 || strcmp(extension, ".jpeg") == 0) {
        return "image/jpeg";
    } else if (strcmp(extension, ".gif") == 0) {
        return "image/gif";
    } else if (strcmp(extension, ".png") == 0) {
        return "image/png";
    } else if (strcmp(extension, ".css") == 0) {
        return "text/css";
    } else if (strcmp(extension, ".txt") == 0 || strcmp(extension, ".py") == 0) {
        return "text/plain";
    } else {
        return "application/octet-stream";
    }
}

char *get_path() {
    char *path = malloc(BUFFER_SIZE);

    if (getcwd(path, BUFFER_SIZE) == NULL) {
        perror("Failed to get current working directory.");
        return NULL;
    }

    strcat(path, "/");

    return path;
}

void handle_get_request(int client_socket, char *path) {
    char buffer[BUFFER_SIZE] = {0};

    if (strcmp(path, "/") == 0) {
        send_server_info(client_socket, "200 OK", "text/plain");
        send(client_socket, server_info, strlen(server_info), 0);
        return;
    } else {
        char *filename = strrchr(path, '/') + 1;
        char full_path[BUFFER_SIZE];
        char *base_path = get_path();
        snprintf(full_path, sizeof(full_path), "%s%s", base_path, filename);
        printf("Opening file: %s\n", full_path);

        FILE *file = fopen(full_path, "r");
        if (file != NULL) {
            const char *content_type = get_content_type(full_path);
            send_server_info(client_socket, "200 OK", content_type);
            while (fgets(buffer, sizeof(buffer), file) != NULL) {
                send(client_socket, buffer, strlen(buffer), 0);
            }
            fclose(file);
        } else {
            send_server_info(client_socket, "404 Not Found", "text/plain");
        }
    }
}

void handle_post_request(int client_socket, char *path) {
    char *filename = strrchr(path, '/') + 1;
    char full_path[BUFFER_SIZE];
    char *base_path = get_path();
    snprintf(full_path, sizeof(full_path), "%s%s", base_path, filename);
    printf("Opening file: %s\n", full_path);

    FILE *file = fopen(full_path, "r");
    if (file != NULL) {
        char *python_output = execute_python_script(full_path);
        if (python_output != NULL) {
            const char *content_type = get_content_type(full_path);
            send_server_info(client_socket, "200 OK", content_type);
            if (strcmp(path, "/") == 0) {
                send(client_socket, server_info, strlen(server_info), 0);
            }
            send(client_socket, python_output, strlen(python_output), 0);
        } else {
            send_server_info(client_socket, "500 Internal Server Error", "text/plain");
        }
        fclose(file);
    } else {
        send_server_info(client_socket, "404 Not Found", "text/plain");
    }
}

void handle_connection(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};

    read(client_socket, buffer, BUFFER_SIZE);

    char method[5];
    char path[BUFFER_SIZE];
    sscanf(buffer, "%s %s", method, path);

    char *extension = strrchr(path, '.');
    int is_python_script = extension && strcmp(extension, ".py") == 0;

    if (strcmp(method, "GET") == 0) {
        handle_get_request(client_socket, path);
    } else if (strcmp(method, "POST") == 0 && is_python_script) {
        handle_post_request(client_socket, path);
    } else {
        char response[BUFFER_SIZE] = {0};
        snprintf(response, sizeof(response),
                 "HTTP/1.0 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nBad "
                 "Request.");
        send(client_socket, response, strlen(response), 0);
    }

    close(client_socket);
}

int create_server_socket() {
    int server_fd;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Failed to create socket.");
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

void set_server_options(int server_fd) {
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        perror("Failed to set socket options.");
        exit(EXIT_FAILURE);
    }
}

void bind_server_socket(int server_fd, struct sockaddr_in *address) {
    if (bind(server_fd, (struct sockaddr *)address, sizeof(*address)) == -1) {
        perror("Failed to bind socket.");
        exit(EXIT_FAILURE);
    }
}

void listen_for_connections(int server_fd) {
    if (listen(server_fd, MAX_CONNECTIONS) == -1) {
        perror("Failed to listen for connections.");
        exit(EXIT_FAILURE);
    }
}

int accept_connection(int server_fd, struct sockaddr_in *address) {
    int address_len = sizeof(*address);
    int client_socket;
    if ((client_socket = accept(server_fd, (struct sockaddr *)address, (socklen_t *)&address_len)) == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    return client_socket;
}

void handle_client_connection(int client_socket, int server_fd) {
    pid_t pid = fork();
    if (pid == 0) {
        close(server_fd);
        handle_connection(client_socket);
        exit(EXIT_SUCCESS);
    } else if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else {
        close(client_socket);
    }
}

int set_port(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Error: The program must be called with two arguments.\n");
        return -1;
    }

    if (strcmp(argv[1], "-p") != 0) {
        printf("Error: The first argument must be -p.\n");
        return -1;
    }

    return atoi(argv[2]);
}

int main(int argc, char *argv[]) {
    int port = set_port(argc, argv);

    if (port == -1) {
        return -1;
    }

    printf("Server running on port %d.\n", port);

    int client_socket;
    struct sockaddr_in address;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    server_fd = create_server_socket();
    set_server_options(server_fd);
    bind_server_socket(server_fd, &address);
    listen_for_connections(server_fd);

    signal(SIGINT, &exit_server);
    setup_sigchld_handler();

    while (1) {
        printf("\nWaiting for a connection...\n\n");
        client_socket = accept_connection(server_fd, &address);
        handle_client_connection(client_socket, server_fd);
    }

    return 0;
}
