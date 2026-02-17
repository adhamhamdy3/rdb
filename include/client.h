#ifndef CLIENT_H
#define CLIENT_H

#include "logger.h"
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

struct current_server {
    struct sockaddr_in address;
};

struct Client {
    int socket;
    current_server curr_server;
};

void init_client_socket(Client& client);
void client_connect_to(Client& client, u_int32_t ip_address, uint16_t port_number);

int32_t send_request(Client const& client, uint8_t const* data, size_t len);
int32_t recv_response(Client const& client);
void close_connection(Client const& client);
void check_err(int ret_val, Client const& client);

#endif
