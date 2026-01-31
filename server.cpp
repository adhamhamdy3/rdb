#include <cassert>
#include <errno.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

const size_t MAX_MSG_LENGTH = 4098; // 4 bytes

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

int32_t read_n(int tcp_connection, char *rbuffer, size_t n) {
    int32_t total_read = 0;

    while (total_read < n) {
        ssize_t ret_val = read(tcp_connection, rbuffer + total_read, n - total_read);

        if (ret_val > 0) {
            total_read += ret_val;
        } else if (ret_val == 0) { // EOF
            return total_read;
        } else { // error
            if (errno == EINTR)
                continue; // retry
            return -1;
        }
    }

    return total_read;
}

int32_t write_n(int tcp_connection, const char *wbuffer, size_t n) {
    int32_t total_write = 0;

    while (total_write < n) {
        ssize_t ret_val = write(tcp_connection, wbuffer + total_write, n - total_write);
        if (ret_val > 0) { // error or EOF
            total_write += ret_val;
        } else if (ret_val == 0) {
            return total_write;
        } else {
            if (errno == EINTR)
                continue;
            return -1;
        }
    }

    return total_write;
}

int32_t process_one_req(int tcp_connection) {
    // 4 bytes message length + up to 4096 bytes payload
    char rbuffer[4 + MAX_MSG_LENGTH] = {};

    errno = 0;
    // read first 4 bytes, message length
    ssize_t ret_val = read_n(tcp_connection, rbuffer, 4);
    if (ret_val != 4) {
        alert(errno == 0 ? "EOF" : "read(): error");
        return -1;
    }

    // read message length, first 4 bytes
    uint32_t len = 0;
    memcpy(&len, rbuffer, 4);
    if (len > MAX_MSG_LENGTH) {
        alert("payload is too long");
        return -1;
    }

    // read the request payload
    ret_val = read_n(tcp_connection, rbuffer + 4, len);
    if (ret_val != len) {
        alert(errno == 0 ? "EOF" : "read(): error");
        return -1;
    }

    // TODO: process the request
    printf("client says: %.*s\n", len, &rbuffer[4]);

    const char reply[] = "world";
    char wbuffer[4 + sizeof(reply)];

    len = (uint32_t)strlen(reply);
    memcpy(wbuffer, &len, 4); // first 4 bytes of wbuffer contains the msg len
    memcpy(&wbuffer[4], reply, len);

    ret_val = write_n(tcp_connection, wbuffer, 4 + len);
    if (ret_val < 0) {
        report_error("write()");
    }

    return 0;
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

        // accept() blocks until a client connects
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &addrlen);
        if (client_socket < 0) { // connection failed
            continue;
        }

        while (true) {
            int32_t err = process_one_req(client_socket);
            if (err) {
                break;
            }
        }

        close(client_socket);
    }

    return 0;
}