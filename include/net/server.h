#ifndef SERVER_H
#define SERVER_H

// Networking
#include <fcntl.h>
#include <netinet/ip.h>
#include <poll.h>
#include <unistd.h>

// clibs
#include <cassert>

// Utils
#include "io/buffer_util.h"
#include "log/logger.h"

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

size_t const MAX_MSG_LENGTH = (32 << 20);
size_t const MAX_ARGS = 200 * 1000;

struct connection_state {
    int tcp_socket = -1;

    bool want_read = false;
    bool want_write = false;
    bool want_close = false;

    std::vector<uint8_t> incoming; // incoming data for the application logic to process
    std::vector<uint8_t> outgoing; // outgoing data from the application logic to send
};

struct Response {
    uint32_t status = 0;
    std::vector<uint8_t> data;
};

int32_t parse_req(uint8_t const* request, uint32_t size, std::vector<std::string>& command);
void process_command(std::vector<std::string>& command, Response& resp);

void make_response(Response const& resp, std::vector<uint8_t>& outgoing);
bool try_one_request(connection_state* conn);

connection_state* handle_accept(int tcp_socket);
void handle_read(connection_state* conn);
void handle_write(connection_state* conn);

struct Server {
    int socket;
    struct sockaddr_in address;

    // this maps each socket to its connection state
    /// fds (sockets) on unix are allocated to the smallest available non-negative integer
    /// so mapping using simple arrays could not be more efficient
    std::vector<connection_state*> conn_state_map;

    std::vector<struct pollfd> sockets_list;
};

void init_server_socket(Server& server);
void init_server_address(Server& server, uint16_t port_number, uint32_t ip_address);
void server_start_listen(Server& server);
void event_loop(Server& server);

#endif
