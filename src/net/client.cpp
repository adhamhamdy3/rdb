#include "net/client.h"

void init_client_socket(Client& client)
{
    // create tcp socket
    client.socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client.socket < 0) {
        Logger::log_error("socket()");
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
        Logger::log_error("connect()");
    }
}

int32_t send_request(Client const& client, std::vector<std::string> const& command)
{
    Buffer out;
    Protocol::serialize_request(command, out);

    return NetworkIO::write_n(client.socket, out.data.data(), out.data.size());
}

int32_t recv_response(Client const& client)
{
    Buffer rbuffer;
    rbuffer.data.assign(4, {});

    errno = 0;
    int32_t ret_val = NetworkIO::read_n(client.socket, rbuffer.data.data(), 4);
    if (ret_val != 4) {
        Logger::alert(errno == 0 ? "EOF" : "read() error");
        return ret_val;
    }

    uint32_t len = 0;
    memcpy(&len, rbuffer.data.data(), 4);
    if (len > CMAX_MSG_LENGTH) {
        Logger::alert("payload is too long");
        return -1;
    }

    // resize the rbuffer to fit the incoming response
    rbuffer.data.resize(4 + len);

    ret_val = NetworkIO::read_n(client.socket, &rbuffer.data[4], len);
    if (ret_val != len) {
        Logger::alert("read(): error");
        return -1;
    }

    return Protocol::deserialize_response(rbuffer);
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
