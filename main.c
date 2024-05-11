#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 30

char* execute_python_script(char* script_path) {
    FILE *python_output;
    char python_command[BUFFER_SIZE];
    static char python_buffer[BUFFER_SIZE * BUFFER_SIZE];
    int status;
    pid_t pid;

    snprintf(python_command, sizeof(python_command), "python3 %s", script_path);
    python_output = popen(python_command, "r");
    if (python_output == NULL) {
        perror("failed to run python script");
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
            perror("failed to close python script process");
        }
    }

    return python_buffer;
}

void sigchld_handler() {
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void setup_sigchld_handler() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

int create_server_socket() {
    int server_fd;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    return server_fd;
}

void set_server_options(int server_fd) {
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        perror("socket options failed");
        exit(EXIT_FAILURE);
    }
}

void bind_server_socket(int server_fd, struct sockaddr_in *address) {
    if (bind(server_fd, (struct sockaddr *)address, sizeof(*address)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
}

void listen_for_connections(int server_fd) {
    if (listen(server_fd, MAX_CONNECTIONS) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
}

int accept_connection(int server_fd, struct sockaddr_in *address) {
    int address_len = sizeof(*address);
    int client_socket;
    if ((client_socket = accept(server_fd, (struct sockaddr *)address, (socklen_t*)&address_len)) == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    return client_socket;
}
