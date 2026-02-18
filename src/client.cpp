#include "client.h"

inline void buffer_append(std::vector<uint8_t>& buffer, uint8_t const* data, size_t len)
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

void init_client_socket(Client& client)
{
    // create tcp socket
    client.socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client.socket < 0) {
        log_error("socket()");
    }
}

void client_connect_to(Client& client, u_int32_t ip_address, uint16_t port_number)
{
    // server address to connect to
    client.curr_server.address.sin_family = AF_INET; // IPv4
    client.curr_server.address.sin_port = htons(port_number);
    client.curr_server.address.sin_addr.s_addr = ntohl(ip_address);

    int ret_val = connect(client.socket, (const struct sockaddr*)&client.curr_server.address, sizeof(client.curr_server.address));
    if (ret_val) {
        log_error("connect()");
    }
}

int32_t send_request(Client const& client, std::vector<std::string> const& command)
{
    uint32_t total_command_size = 4;
    for (std::string const str : command) {
        total_command_size += 4 + str.size();
    }

    if (total_command_size > MAX_MSG_LENGTH) {
        return -1;
    }

    char wbuffer[4 + MAX_MSG_LENGTH];
    memcpy(&wbuffer[0], &total_command_size, 4);

    uint32_t n = command.size();
    memcpy(&wbuffer[4], &n, 4);

    size_t curr_pos = 8;

    for (std::string const& str : command) {
        uint32_t str_size = (uint32_t)str.size();

        memcpy(&wbuffer[curr_pos], &str_size, 4);
        curr_pos += 4;

        memcpy(&wbuffer[curr_pos], str.data(), str.size());
        curr_pos += str_size;
    }

    return write_n(client.socket, (uint8_t const*)&wbuffer, 4 + total_command_size);
}

int32_t recv_response(Client const& client)
{
    std::vector<uint8_t> rbuffer(4);

    errno = 0;
    int32_t ret_val = read_n(client.socket, rbuffer.data(), 4);
    if (ret_val != 4) {
        alert(errno == 0 ? "EOF" : "read() error");
        return ret_val;
    }

    uint32_t len = 0;
    memcpy(&len, rbuffer.data(), 4);
    if (len > MAX_MSG_LENGTH) {
        alert("payload is too long");
        return -1;
    }

    // resize the rbuffer to fit the incoming response
    rbuffer.resize(4 + len);

    ret_val = read_n(client.socket, &rbuffer[4], len);
    if (ret_val != len) {
        alert("read(): error");
        return -1;
    }

    uint32_t status = 0;
    memcpy(&status, &rbuffer[4], 4);
    printf("server says [status=%u]: %.*s\n", status, (int)(len - 4), &rbuffer[8]);

    return 0;
}

void close_connection(Client const& client)
{
    close(client.socket);
}

void check_err(int ret_val, Client const& client)
{
    if (ret_val < 0) {
        close_connection(client);
        exit(1);
    }
}
