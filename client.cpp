#include <arpa/inet.h>
#include <cassert>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

const size_t MAX_MSG_LENGTH = 4098; // 4 bytes

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

int32_t send_request(int tcp_connection, const char *data) {
    uint32_t len = (uint32_t)strlen(data);
    if (len > MAX_MSG_LENGTH) {
        return -1;
    }

    char wbuffer[4 + MAX_MSG_LENGTH];

    memcpy(wbuffer, &len, 4);
    memcpy(wbuffer + 4, data, len);

    // send request
    int32_t ret_val = write_n(tcp_connection, wbuffer, 4 + len);
    if (ret_val < 0) {
        report_error("write()");
    }

    return ret_val;
}

int32_t recv_response(int tcp_connection) {
    char rbuffer[4 + MAX_MSG_LENGTH];

    errno = 0;
    int32_t ret_val = read_n(tcp_connection, rbuffer, 4);
    if (ret_val != 4) {
        alert(errno == 0 ? "EOF" : "read() error");
        return -1;
    }

    int32_t len = 0;

    memcpy(&len, rbuffer, 4);
    if (len > MAX_MSG_LENGTH) {
        alert("payload is too long");
        return -1;
    }

    ret_val = read_n(tcp_connection, &rbuffer[4], len);
    if (ret_val != len) {
        alert("read(): error");
        return -1;
    }
    printf("server says: %.*s\n", len, &rbuffer[4]);
    return 0;
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

    /*
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
    */

    int32_t ret_val = send_request(client_socket, "hello1");

    ret_val = send_request(client_socket, "hello2");

    // close connection
    close(client_socket);

    return 0;
}