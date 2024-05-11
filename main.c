#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX_CONNECTIONS 30

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
