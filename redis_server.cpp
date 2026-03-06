#include "net/server.h"

int main(int argc, char* argv[])
{
    uint16_t port = 1234;
    uint32_t ip = 0; // INADDR_ANY

    for (int i = 1; i < argc; ++i) {
        if ((strcmp(argv[i], "--port") == 0 || strcmp(argv[i], "-p") == 0) && i + 1 < argc) {
            port = static_cast<uint16_t>(std::atoi(argv[++i]));
        } else if ((strcmp(argv[i], "--host") == 0 || strcmp(argv[i], "-h") == 0) && i + 1 < argc) {
            if (inet_pton(AF_INET, argv[++i], &ip) != 1) {
                fprintf(stderr, "invalid address: %s\n", argv[i]);
                return 1;
            }
            ip = ntohl(ip);
        } else {
            fprintf(stderr, "usage: %s [--port|-p <port>] [--host|-h <ip>]\n", argv[0]);
            return 1;
        }
    }

    Server server = {};
    init_server_socket(server);
    init_server_address(server, port, ip);
    server_start_listen(server);

    return 0;
}
