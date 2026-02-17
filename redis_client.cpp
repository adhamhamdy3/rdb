#include "client.h"

int main(int argc, char* argv[])
{
    Client client = {};
    init_client_socket(client);
    client_connect_to(client, INADDR_LOOPBACK, 1234);

    std::vector<std::string> requests = {"adham", "hamdy", "hamed", "abdulhamdeid"};
    for (auto const& req : requests) {
        auto ret_val = send_request(client, (uint8_t*)req.data(), req.size());

        check_err(ret_val, client);
    }

    for (size_t i = 0; i < requests.size(); i++) {
        auto ret_val = recv_response(client);

        check_err(ret_val, client);
    }

    close_connection(client);

    return 0;
}
