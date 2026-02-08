#include <arpa/inet.h>
#include <cassert>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

size_t const MAX_MSG_LENGTH = (32 << 20);

void alert(char const* msg)
{
    fprintf(stderr, "%s\n", msg);
}

void report_error(char const* msg)
{
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

void buffer_append(std::vector<uint8_t>& buffer, uint8_t const* data, size_t len)
{
    // append to back
    buffer.insert(buffer.end(), data, data + len);
}

int32_t read_n(int tcp_connection, uint8_t* rbuffer, size_t n)
{
    int32_t total_read = 0;

    while (total_read < (int32_t)n) {
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

int32_t write_n(int tcp_connection, uint8_t const* wbuffer, size_t n)
{
    int32_t total_write = 0;

    while (total_write < (int32_t)n) {
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

int32_t send_request(int tcp_connection, uint8_t const* data, size_t len)
{
    if (len > MAX_MSG_LENGTH) {
        return -1;
    }

    std::vector<uint8_t> wbuffer;

    uint32_t msg_len = (uint32_t)len;
    buffer_append(wbuffer, (uint8_t const*)&msg_len, 4);
    buffer_append(wbuffer, data, len);

    // send request
    int32_t ret_val = write_n(tcp_connection, wbuffer.data(), 4 + len);
    if (ret_val < 0) {
        report_error("write()");
    }

    return ret_val;
}

int32_t recv_response(int tcp_connection)
{
    std::vector<uint8_t> rbuffer(4);

    errno = 0;
    int32_t ret_val = read_n(tcp_connection, rbuffer.data(), 4);
    if (ret_val != 4) {
        alert(errno == 0 ? "EOF" : "read() error");
        return -1;
    }

    uint32_t len = 0;
    memcpy(&len, rbuffer.data(), 4);
    if (len > MAX_MSG_LENGTH) {
        alert("payload is too long");
        return -1;
    }

    // resize the rbuffer to fit the incoming response
    rbuffer.resize(4 + len);

    ret_val = read_n(tcp_connection, &rbuffer[4], len);
    if (ret_val != len) {
        alert("read(): error");
        return -1;
    }
    printf("server says: %.*s\n", len, &rbuffer[4]);

    return 0;
}

void check_err(int ret_val, int socket)
{
    if (ret_val < 0) {
        close(socket);
        exit(1);
    }
}

int main()
{
    // create tcp socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        report_error("socket()");
    }

    // server address to connect to
    struct sockaddr_in server_address = {};
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_port = htons(1234);
    server_address.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // loopback ip address: 127.0.0.1

    int result_value = connect(client_socket, (const struct sockaddr*)&server_address, sizeof(server_address));
    if (result_value) {
        report_error("connect()");
    }

    std::vector<std::string> requests = { "adham", "hamdy", "hamed", "abdulhameid", std::string("z", 1000) };
    for (auto const& req : requests) {
        auto ret_val = send_request(client_socket, (uint8_t*)req.data(), req.size());

        check_err(ret_val, client_socket);
    }

    for (size_t i = 0; i < requests.size(); i++) {
        auto ret_val = recv_response(client_socket);

        check_err(ret_val, client_socket);
    }

    // close connection
    close(client_socket);

    return 0;
}
