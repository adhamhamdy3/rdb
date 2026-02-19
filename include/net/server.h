#ifndef SERVER_H
#define SERVER_H

// Networking
#include "protocol.h"
#include <fcntl.h>
#include <netinet/ip.h>
#include <poll.h>
#include <unistd.h>

// clibs
#include <cassert>

// Utils
#include "io/buffer_util.h"
#include "log/logger.h"

// Storage
#include "storage/rdb.h"

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
    int tcp_socket = -1;

    bool want_read = false;
    bool want_write = false;
    bool want_close = false;

    std::vector<uint8_t> incoming; // incoming data for the application logic to process
    std::vector<uint8_t> outgoing; // outgoing data from the application logic to send
};

int32_t parse_req(uint8_t const* request, uint32_t size, std::vector<std::string>& command);
void process_command(std::vector<std::string> const& command, Response& resp, Database& db);

void make_response(Response const& resp, std::vector<uint8_t>& outgoing);
bool try_one_request(connection_state* conn, Database& db);

connection_state* handle_accept(int tcp_socket);
void handle_read(connection_state* conn, Database& db);
void handle_write(connection_state* conn);

struct Server {
    int socket;
    struct sockaddr_in address;

    // this maps each socket to its connection state
    /// fds (sockets) on unix are allocated to the smallest available non-negative integer
    /// so mapping using simple arrays could not be more efficient
    std::vector<connection_state*> conn_state_map;

    std::vector<struct pollfd> sockets_list;

    Database db;
};

void init_server_socket(Server& server);
void init_server_address(Server& server, uint16_t port_number, uint32_t ip_address);
void server_start_listen(Server& server);
void event_loop(Server& server);

#endif
