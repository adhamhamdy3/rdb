#include <errno.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/*
struct sockaddr_in {
  uint16_t sin_family;      // which IP version
  uint16_t sin_port;        // port number
  struct in_addr sin_addr;  // ip address
};
*/

/*
struct in_addr {
  uint32_t s_addr;  // ip address
};
*/

void alert(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

void report_error(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

void process(int tcp_connection) {
    char rbuffer[64] = {};

    ssize_t ret_val = read(tcp_connection, rbuffer, sizeof(rbuffer) - 1);

    if (ret_val < 0) {
        alert("read(): error");
        return;
    }

    printf("client says: %s\n", rbuffer);

    char wbuffer[] = "world";

    ret_val = write(tcp_connection, wbuffer, strlen(wbuffer));
    if (ret_val < 0) {
        report_error("write()");
    }
}

int main() {
    // create tcp socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        report_error("socket()");
    }

    int reuse_option = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse_option, sizeof(reuse_option));

    // server address (IP + port)
    struct sockaddr_in server_address = {};

    // htons & htonl swaps from big-endian to little-endian
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_port = htons(1234);
    server_address.sin_addr.s_addr = htonl(0); // IP 0.0.0.0

    int result_value = bind(server_socket, (const struct sockaddr *)&server_address, sizeof(server_address));
    if (result_value) {
        report_error("bind()");
    }

    // listen
    result_value = listen(server_socket, SOMAXCONN); // SOMAXCONN: size of the queue
    if (result_value) {
        report_error("listen()");
    }

    // accept connections
    while (true) {
        struct sockaddr_in client_address = {};
        socklen_t addrlen = sizeof(client_address);

        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &addrlen);

        if (client_socket < 0) { // connection failed
            continue;
        }

        process(client_socket);

        close(client_socket);
    }

    return 0;
}