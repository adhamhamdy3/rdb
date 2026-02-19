#include "net/server.h"
#include <iostream>

int main(int argc, char* argv[])
{
    Server server = {};
    init_server_socket(server);
    init_server_address(server, 1234, 0);
    server_start_listen(server);

    return 0;
}
