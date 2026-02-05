#include <cassert>
#include <errno.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

size_t const MAX_MSG_LENGTH = (32 << 20);

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

/*
struct pollfd {
  int fd;

  int events;   // requested flags: POLLIN, POLLOUT, and POLLERR
  int revents;  // returned allowed events: you can read/write
};
*/

struct connection_state {
    int tcp_socekt = -1;

    bool want_read = false;
    bool want_write = false;
    bool want_close = false;

    std::vector<int8_t> incoming; // incoming data for the application logic to process
    std::vector<int8_t> outgoing; // outgoing data from the application logic to send
};

void alert(char const* msg)
{
    fprintf(stderr, "%s\n", msg);
}

void log_error(char const* msg)
{
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

void buffer_append(std::vector<int8_t>& buffer, int8_t const* data, size_t len)
{
    // append to back
    buffer.insert(buffer.end(), data, data + len);
}

void buffer_consume(std::vector<int8_t>& buffer, size_t len)
{
    // remove from front
    buffer.erase(buffer.begin() + len);
}

void socket_set_nb(int socket)
{
    int flags = fcntl(socket, F_GETFL, 0);

    // O_NONBLOCK flag applies to: read() - write() - accept() - connect()
    flags |= O_NONBLOCK;
    fcntl(socket, F_SETFL, flags);
}

bool try_one_request(connection_state* conn)
{
    // message size
    if (conn->incoming.size() < 4) {
        return false;
    }

    u_int32_t len = 0;
    memcpy(&len, conn->incoming.data(), 4);
    if (len > MAX_MSG_LENGTH) {
        alert("message too long");
        conn->want_close = true;
        return false;
    }

    // message body
    if (4 + len > conn->incoming.size()) {
        return false;
    }

    int8_t* request = &conn->incoming[4];

    // TODO: process the request
    printf("client says: len:%d data:%.*s\n",
        len, len < 100 ? len : 100, request);

    // send the response to outgoing buffer
    buffer_append(conn->outgoing, (int8_t const*)&len, 4);
    buffer_append(conn->outgoing, request, len);

    // remove the message from the incoming buffer
    buffer_consume(conn->incoming, 4 + len);

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
    conn->tcp_socekt = connection_socket;
    conn->want_read = true; // read the first request

    return conn;
}

// app logic when socket is writable
void handle_write(connection_state* conn)
{
    assert(conn->outgoing.size() > 0);

    int ret_val = write(conn->tcp_socekt, conn->outgoing.data(), conn->outgoing.size());
    if (ret_val < 0 && errno == EAGAIN) {
        return; // not ready
    }

    if (ret_val < 0) {
        alert("write() error");
        conn->want_close = true;
        return;
    }

    // remove written data from the outgoing buffer
    buffer_consume(conn->outgoing, ret_val);

    // state transition
    if (conn->outgoing.size() == 0) {
        conn->want_read = true;
        conn->want_write = false;
    }
}

// app logic when socket is readable
void handle_read(connection_state* conn)
{
    int8_t buffer[64 * 1024];
    ssize_t ret_val = read(conn->tcp_socekt, buffer, sizeof(buffer));
    if (ret_val < 0 && errno == EAGAIN) {
        return; // not ready
    }

    if (ret_val < 0) {
        alert("read() error");
        conn->want_close = true;
        return;
    }

    // handle EOF
    if (ret_val == 0) {
        if (conn->incoming.size() == 0) {
            alert("client closed");
        } else {
            alert("unexpected EOF");
        }
        conn->want_close = true;
        return;
    }

    // accumulate the new data to connection state incoming data buffer
    buffer_append(conn->incoming, buffer, ret_val);

    // try to parse the accumulated data
    while (try_one_request(conn)) {
        /* do nothing */
    }

    // state transition
    if (conn->outgoing.size() > 0) {
        conn->want_write = true;
        conn->want_read = false;
        return handle_write(conn);
    }
}

int main()
{
    // create tcp socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        log_error("socket()");
    }

    int reuse_option = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse_option, sizeof(reuse_option));

    // server address (IP + port)
    struct sockaddr_in server_address = {};

    // htons & htonl swaps from big-endian to little-endian
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_port = htons(1234);
    server_address.sin_addr.s_addr = htonl(0); // IP 0.0.0.0

    int result_value = bind(server_socket, (const struct sockaddr*)&server_address, sizeof(server_address));
    if (result_value) {
        log_error("bind()");
    }

    // set server socket to non-blocking mode
    socket_set_nb(server_socket);

    // listen
    result_value = listen(server_socket, SOMAXCONN); // SOMAXCONN: size of the queue
    if (result_value) {
        log_error("listen()");
    }

    // this maps each socket to its connection state
    /// fds (sockets) on unix are allocated to the smallest available non-negative integer
    /// so mapping using simple arrays could not be more efficient
    std::vector<connection_state*> conn_state_map;

    std::vector<struct pollfd> sockets_list;

    // event loop
    // accept connections
    /// rebuild the sockets list every iteration based on the application logic's intent read/write/error
    while (true) {
        sockets_list.clear();

        struct pollfd socket = { server_socket, POLLIN, 0 }; // index 0: listening socket, non-blocking accpet()
        sockets_list.push_back(socket);

        for (connection_state* conn : conn_state_map) {
            if (!conn) {
                continue;
            }

            struct pollfd socket = { conn->tcp_socekt, POLLERR, 0 }; // always wake up if error happened

            if (conn->want_read) {
                socket.events |= POLLIN;
            }
            if (conn->want_write) {
                socket.events |= POLLOUT;
            }

            sockets_list.push_back(socket);
        }

        /// blocking - check all sockets receive/send buffers or TCP errors
        int ret_val = poll(sockets_list.data(), (nfds_t)sockets_list.size(), -1);
        if (ret_val < -1 && errno == EINTR) { // nothing is ready rn
            continue;
        }
        if (ret_val < 0) {
            log_error("poll()");
        }

        if (sockets_list[0].revents & POLLIN) {
            int listening_socket = sockets_list[0].fd;

            connection_state* conn = handle_accept(listening_socket);
            if (conn) {
                if (conn_state_map.size() <= (size_t)conn->tcp_socekt) {
                    conn_state_map.resize(conn->tcp_socekt + 1, nullptr);
                }
                assert(!conn_state_map[conn->tcp_socekt]);
                conn_state_map[conn->tcp_socekt] = conn;
            }
        }

        // handle connection socket
        for (size_t i = 1; i < sockets_list.size(); i++) {
            connection_state* conn = conn_state_map[sockets_list[i].fd];
            uint32_t revents = sockets_list[i].revents;

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
                close(conn->tcp_socekt);
                conn_state_map[conn->tcp_socekt] = nullptr;
                delete conn;
            }
        }
    }

    return 0;
}
