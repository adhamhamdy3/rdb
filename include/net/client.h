#ifndef CLIENT_H
#define CLIENT_H

// Networking
#include "io/network_io.h"
#include "protocol.h"
#include <arpa/inet.h>

// STLs
#include <string.h>
#include <string>
#include <vector>

// Utils
#include "log/logger.h"

struct current_server {
    struct sockaddr_in address;
};

struct Client {
    int socket;
    current_server curr_server;
};

void init_client_socket(Client& client);
void client_connect_to(Client& client, u_int32_t ip_address, uint16_t port_number);

int32_t send_request(Client const& client, std::vector<std::string> const& command);
int32_t recv_response(Client const& client);

void close_connection(Client const& client);
void check_err(int ret_val, Client const& client);

#endif
