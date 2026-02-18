#include "client.h"

int main(int argc, char* argv[])
{
    Client client = {};
    init_client_socket(client);
    client_connect_to(client, INADDR_LOOPBACK, 1234);

    std::vector<std::string> command;
    for (size_t i = 1; i < argc; i++) {
        command.push_back(argv[i]);
    }

    auto ret_val = send_request(client, command);
    check_err(ret_val, client);

    ret_val = recv_response(client);
    check_err(ret_val, client);

    close_connection(client);

    return 0;
}
