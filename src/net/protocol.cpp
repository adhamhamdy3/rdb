#include "net/protocol.h"

int32_t Protocol::serialize_request(std::vector<std::string> const& command, std::vector<uint8_t>& out)
{
    uint32_t total_command_size = 4;
    for (std::string const str : command) {
        total_command_size += 4 + str.size();
    }

    if (total_command_size > CMAX_MSG_LENGTH) {
        return -1;
    }

    BufferUtil::buffer_append(out, (uint8_t const*)&total_command_size, 4);

    uint32_t n_str = command.size();
    BufferUtil::buffer_append(out, (uint8_t const*)&n_str, 4);

    for (std::string const& str : command) {
        uint32_t str_size = (uint32_t)str.size();

        BufferUtil::buffer_append(out, (uint8_t const*)&str_size, 4);
        BufferUtil::buffer_append(out, (uint8_t const*)str.data(), str.size());
    }

    return 0;
}

int32_t Protocol::deserialize_request(uint8_t const* request, uint32_t size, std::vector<std::string>& command)
{
    uint8_t const* end = request + size;
    uint32_t n_str = 0;

    if (!BufferUtil::read_u32(request, end, n_str)) {
        return -1;
    }

    if (n_str > MAX_ARGS) {
        return -1;
    }

    while (command.size() < n_str) {
        uint32_t len = 0;
        if (!BufferUtil::read_u32(request, end, len)) {
            return -1;
        }

        command.push_back(std::string());
        if (!BufferUtil::read_str(request, end, len, command.back())) {
            return -1;
        }
    }
    if (request != end) {
        return -1;
    }

    return 0;
}

void Protocol::serialize_response(Response const& resp, std::vector<uint8_t>& outgoing)
{
    uint32_t resp_len = (uint32_t)4 + resp.data.size();
    BufferUtil::buffer_append(outgoing, (uint8_t const*)&resp_len, 4);
    BufferUtil::buffer_append(outgoing, (uint8_t const*)&resp.status, 4);
    BufferUtil::buffer_append(outgoing, resp.data.data(), resp.data.size());
}

int32_t Protocol::deserialize_response(std::vector<uint8_t> const& buffer)
{
    // TODO: read32()
    uint32_t len = 0;
    memcpy(&len, buffer.data(), 4);

    uint32_t status = 0;
    memcpy(&status, &buffer[4], 4);
    printf("server says [status=%u]: %.*s\n", status, (int)(len - 4), &buffer[8]);

    return 0;
}
