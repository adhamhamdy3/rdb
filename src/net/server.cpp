#include "net/server.h"

void socket_set_nb(int socket)
{
    int flags = fcntl(socket, F_GETFL, 0);

    // O_NONBLOCK flag applies to: read() - write() - accept() - connect()
    flags |= O_NONBLOCK;
    fcntl(socket, F_SETFL, flags);
}

int32_t parse_req(uint8_t const* request, uint32_t size, std::vector<std::string>& command)
{
    uint8_t const* end = request + size;
    uint32_t nstr = 0;

    if (!BufferUtil::read_u32(request, end, nstr)) {
        return -1;
    }

    if (nstr > MAX_ARGS) {
        return -1;
    }

    while (command.size() < nstr) {
        uint32_t len = 0;
        if (!BufferUtil::read_u32(request, end, len)) {
            return -1;
        }

        command.push_back(std::string());
        if (!BufferUtil::read_str(request, end, len, command.back())) {
            return -1;
        }
    }
    if (request != end) {
        return -1;
    }

    return 0;
}

void process_command(std::vector<std::string>& command, Response& resp)
{
    // TODO: process the command
}

void make_response(Response const& resp, std::vector<uint8_t>& outgoing)
{
    uint32_t resp_len = (uint32_t)4 + resp.data.size();
    BufferUtil::buffer_append(outgoing, (uint8_t const*)&resp_len, 4);
    BufferUtil::buffer_append(outgoing, (uint8_t const*)&resp.status, 4);
    BufferUtil::buffer_append(outgoing, resp.data.data(), resp.data.size());
}

bool try_one_request(connection_state* conn)
{
    // message size
    if (conn->incoming.size() < 4) {
        return false;
    }

    uint32_t len = 0;
    memcpy(&len, conn->incoming.data(), 4);
    if (len > MAX_MSG_LENGTH) {
        Logger::alert("message too long");
        conn->want_close = true;
        return false;
    }

    // message body
    if (4 + len > conn->incoming.size()) {
        return false; // request body is not fully received yet
    }

    uint8_t* request = &conn->incoming[4];

    std::vector<std::string> command;
    if (parse_req(request, len, command) < 0) {
        conn->want_close = true;
        return false;
    }

    Response resp;
    process_command(command, resp);

    // send the response to outgoing buffer
    make_response(resp, conn->outgoing);

    // remove the message from the incoming buffer
    BufferUtil::buffer_consume(conn->incoming, 4 + len);

    return true;
}

connection_state* handle_accept(int tcp_socket)
{
    struct sockaddr_in client_socket = {};
    socklen_t client_address = sizeof(client_socket);

    int connection_socket = accept(tcp_socket, (struct sockaddr*)&client_socket, &client_address);
    if (connection_socket < 1) {
        return nullptr;
    }

    uint32_t ip = client_socket.sin_addr.s_addr;
    fprintf(stderr, "new client from %u.%u.%u.%u:%u\n",
        ip & 255, (ip >> 8) & 255, (ip >> 16) & 255, ip >> 24,
        ntohs(client_socket.sin_port));

    // set this connection socket to non-blocking
    socket_set_nb(connection_socket);

    // create connection state for this socket
    connection_state* conn = new connection_state();
    conn->tcp_socket = connection_socket;
    conn->want_read = true; // read the first request

    return conn;
}

// app logic when socket is readable
void handle_read(connection_state* conn)
{
    uint8_t buffer[64 * 1024];
    ssize_t ret_val = read(conn->tcp_socket, buffer, sizeof(buffer));
    if (ret_val < 0 && errno == EAGAIN) {
        return; // not ready
    }

    if (ret_val < 0) {
        Logger::alert("read() error");
        conn->want_close = true;
        return;
    }

    // handle EOF
    if (ret_val == 0) {
        if (conn->incoming.size() == 0) {
            Logger::alert("client closed");
        } else {
            Logger::alert("unexpected EOF");
        }
        conn->want_close = true;
        return;
    }

    // accumulate the new data to connection state incoming data buffer
    BufferUtil::buffer_append(conn->incoming, buffer, ret_val);

    // try to parse the accumulated data
    while (try_one_request(conn)) {
        /* do nothing */
    }

    // state transition
    if (conn->outgoing.size() > 0) {
        conn->want_write = true;
        conn->want_read = false;

        // Instead of waiting for the next loop iteration (extra syscall) and a POLLOUT
        // try to write now
        return handle_write(conn); // optimization
    }
}

// app logic when socket is writable
void handle_write(connection_state* conn)
{
    assert(conn->outgoing.size() > 0);

    int ret_val = write(conn->tcp_socket, conn->outgoing.data(), conn->outgoing.size());
    if (ret_val < 0 && errno == EAGAIN) {
        return; // not ready
    }

    if (ret_val < 0) {
        Logger::alert("write() error");
        conn->want_close = true;
        return;
    }

    // remove written data from the outgoing buffer
    BufferUtil::buffer_consume(conn->outgoing, ret_val);

    // state transition
    if (conn->outgoing.size() == 0) {
        conn->want_read = true;
        conn->want_write = false;
    }
}

void init_server_socket(Server& server)
{
    // create tcp socket
    server.socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server.socket < 0) {
        Logger::log_error("socket()");
    }

    int reuse_option = 1;
    setsockopt(server.socket, SOL_SOCKET, SO_REUSEADDR, &reuse_option, sizeof(reuse_option));
}

void init_server_address(Server& server, uint16_t port_number, uint32_t ip_address)
{
    // server address (IP + port)
    // htons & htonl swaps from big-endian to little-endian
    server.address.sin_family = AF_INET; // IPv4
    server.address.sin_port = htons(port_number);
    server.address.sin_addr.s_addr = htonl(ip_address);

    int ret_val = bind(server.socket, (const struct sockaddr*)&server.address, sizeof(server.address));
    if (ret_val) {
        Logger::log_error("bind()");
    }

    // set server socket to non-blocking mode
    socket_set_nb(server.socket);
}

void server_start_listen(Server& server)
{
    // listen
    int ret_val = listen(server.socket, SOMAXCONN); // SOMAXCONN: size of the queue
    if (ret_val) {
        Logger::log_error("listen()");
    }

    Logger::alert("server is listening...");

    event_loop(server);
}

void event_loop(Server& server)
{
    // event loop
    // accept connections
    /// rebuild the sockets list every iteration based on the application logic's intent read/write/error
    while (true) {
        server.sockets_list.clear();

        struct pollfd socket = { server.socket, POLLIN, 0 }; // index 0: listening socket, non-blocking accept()
        server.sockets_list.push_back(socket);

        for (connection_state* conn : server.conn_state_map) {
            if (!conn) {
                continue;
            }

            struct pollfd sock = { conn->tcp_socket, POLLERR, 0 }; // always wake up if error happened

            if (conn->want_read) {
                sock.events |= POLLIN;
            }
            if (conn->want_write) {
                sock.events |= POLLOUT;
            }

            server.sockets_list.push_back(sock);
        }

        /// blocking - check all sockets receive/send buffers or TCP errors
        int ret_val = poll(server.sockets_list.data(), (nfds_t)server.sockets_list.size(), -1);
        if (ret_val < 0 && errno == EINTR) { // nothing is ready rn
            continue;
        }
        if (ret_val < 0) {
            Logger::log_error("poll()");
        }

        if (server.sockets_list[0].revents & POLLIN) {
            int listening_socket = server.sockets_list[0].fd;

            connection_state* conn = handle_accept(listening_socket);
            if (conn) {
                if (server.conn_state_map.size() <= (size_t)conn->tcp_socket) {
                    server.conn_state_map.resize(conn->tcp_socket + 1, nullptr);
                }

                assert(!server.conn_state_map[conn->tcp_socket]);
                server.conn_state_map[conn->tcp_socket] = conn;
            }
        }

        // handle connection socket
        for (size_t i = 1; i < server.sockets_list.size(); i++) {
            connection_state* conn = server.conn_state_map[server.sockets_list[i].fd];
            uint32_t revents = server.sockets_list[i].revents;

            if (revents == 0) {
                continue;
            }

            if (revents & POLLIN) {
                assert(conn->want_read);
                handle_read(conn);
            }

            if (revents & POLLOUT) {
                assert(conn->want_write);
                handle_write(conn);
            }

            if ((revents & POLLERR) || conn->want_close) {
                close(conn->tcp_socket);
                server.conn_state_map[conn->tcp_socket] = nullptr;
                delete conn;
            }
        }
    }
}
