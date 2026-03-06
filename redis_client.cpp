#include "net/client.h"

int main(int argc, char* argv[])
{
    uint16_t port = 1234;
    uint32_t ip = INADDR_LOOPBACK;
    int cmd_start = 1; // index where the actual command args begin

    for (int i = 1; i < argc; ++i) {
        if ((strcmp(argv[i], "--port") == 0 || strcmp(argv[i], "-p") == 0) && i + 1 < argc) {
            port = static_cast<uint16_t>(std::atoi(argv[++i]));
            cmd_start = i + 1;
        } else if ((strcmp(argv[i], "--host") == 0 || strcmp(argv[i], "-h") == 0) && i + 1 < argc) {
            uint32_t parsed = 0;
            if (inet_pton(AF_INET, argv[++i], &parsed) != 1) {
                fprintf(stderr, "invalid address: %s\n", argv[i]);
                return 1;
            }
            ip = ntohl(parsed); // client_connect_to calls ntohl, so pass host-order
            cmd_start = i + 1;
        } else {
            break; // remaining args are the command
        }
    }

    Client client = {};
    init_client_socket(client);
    client_connect_to(client, ip, port);

    std::vector<std::string> command;
    for (int i = cmd_start; i < argc; ++i) {
        command.push_back(argv[i]);
    }

    auto ret_val = send_request(client, command);
    check_err(ret_val, client);

    ret_val = recv_response(client);
    check_err(ret_val, client);

    close_connection(client);

    return 0;
}
