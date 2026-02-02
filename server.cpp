#include <cassert>
#include <errno.h>
#include <netinet/ip.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

const size_t MAX_MSG_LENGTH = 4098; // 4098 bytes

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

    std::vector<int32_t> incoming; // incoming data for the application logic to process
    std::vector<int32_t> outgoing; // outgoing data from the application logic
};

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

connection_state *handle_accept(int tcp_socket) {
    // TODO:
    return nullptr;
}

int handle_read(connection_state *conn) {
    // TODO:
    return 0;
}

int handle_write(connection_state *conn) {
    // TODO:
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

    // this maps each socket to its connection state
    /// fds (sockets) on unix are allocated to the smallest available non-negative integer
    /// so mapping using simple arrays could not be more efficient
    std::vector<connection_state *> conn_state_map;

    std::vector<struct pollfd> sockets_list;

    // event loop
    // accept connections
    /// rebuild the sockets list every iteration based on the application logic's intent read/write/error
    while (true) {
        sockets_list.clear();

        struct pollfd socket = {server_socket, POLLIN, 0}; // index 0: listening socket, non-blocking accpet()
        sockets_list.push_back(socket);

        for (connection_state *conn : conn_state_map) {
            struct pollfd socket = {conn->tcp_socekt, POLLERR, 0}; // always wake up if error happened

            if (conn->want_read) {
                socket.events |= POLLIN;
            }
            if (conn->want_write) {
                socket.events |= POLLOUT;
            }

            sockets_list.push_back(socket);
        }

        /// blocking - check all sockets receive/send buffers or TCP errors
        int ret_val = poll(sockets_list.data(), sockets_list.size(), -1);
        if (ret_val < -1 && errno == EINTR) { // nothing is ready rn
            continue;
        }
        if (ret_val < -1) {
            report_error("poll()");
        }

        if (sockets_list[0].revents & POLLIN) {
            int listening_socket = sockets_list[0].fd;

            connection_state *conn = handle_accept(listening_socket);
            if (conn) {
                if (conn_state_map.size() <= (size_t)listening_socket) {
                    conn_state_map.resize(listening_socket + 1);
                }
                conn_state_map[listening_socket] = conn;
            }
        }

        // handle connection socket
        for (size_t i = 1; i < sockets_list.size(); i++) {
            connection_state *conn = conn_state_map[sockets_list[i].fd];
            uint32_t revents = sockets_list[i].revents;

            if (revents & POLLIN) {
                handle_read(conn);
            }

            if (revents & POLLOUT) {
                handle_write(conn);
            }

            if (revents & POLLERR || conn->want_close) {
                close(conn->tcp_socekt);
                conn_state_map[conn->tcp_socekt] = nullptr;
                delete conn;
            }
        }
    }

    return 0;
}