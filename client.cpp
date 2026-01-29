#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void alert(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

void report_error(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

int main() {
    // create tcp socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        report_error("socket()");
    }

    // server address to connect to
    struct sockaddr_in server_address = {};
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_port = ntohs(1234);
    server_address.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // loopback ip address: 127.0.0.1

    int result_value = connect(client_socket, (const struct sockaddr *)&server_address, sizeof(server_address));
    if (result_value) {
        report_error("connect()");
    }

    char msg[] = "hello";
    ssize_t bytes_written = write(client_socket, msg, strlen(msg));
    if (bytes_written < 0) {
        report_error("write()");
    }

    char read_buffer[64] = {};
    ssize_t bytes_read = read(client_socket, read_buffer, sizeof(read_buffer) - 1);
    if (bytes_read < 0) {
        report_error("read()");
    }

    printf("server says: %s\n", read_buffer);

    // close connection
    close(client_socket);

    return 0;
}