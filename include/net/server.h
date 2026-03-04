#ifndef SERVER_H
#define SERVER_H

// Networking
#include "net/protocol.h"
#include <fcntl.h>
#include <netinet/ip.h>
#include <poll.h>
#include <unistd.h>

// clibs
#include <cassert>

// Utils
#include "log/logger.h"
#include "timer.h"

// Storage
#include "storage/list.h"
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

uint64_t const MAX_IDLE_TIMEOUT = 5000; // 5 sec

// TODO: separate header
struct connection_state {
    int tcp_socket = -1;

    bool want_read = false;
    bool want_write = false;
    bool want_close = false;

    Buffer incoming; // incoming data for the application logic to process
    Buffer outgoing; // outgoing data from the application logic to send

    // timers
    uint64_t last_active_ms = 0;
    DList idle_node; // intrusive data-structure
};

struct Server {
    int socket;
    struct sockaddr_in address;

    // this maps each socket to its connection state
    /// fds (sockets) on unix are allocated to the smallest available non-negative integer
    /// so mapping using simple arrays could not be more efficient
    std::vector<connection_state*> conn_state_map;
    DList idle_queue;

    std::vector<struct pollfd> sockets_list;

    Database db;
};

void init_server_socket(Server& server);
void init_server_address(Server& server, uint16_t port_number, uint32_t ip_address);
void server_start_listen(Server& server);
void event_loop(Server& server);

// conn_state_init
void conn_destroy(Server& server, connection_state* conn);

void process_command(std::vector<std::string> const& command, Buffer& buf, Database& db);

bool try_one_request(connection_state* conn, Database& db);

connection_state* handle_accept(Server& server, int tcp_socket);
void handle_read(connection_state* conn, Database& db);
void handle_write(connection_state* conn);

#endif
