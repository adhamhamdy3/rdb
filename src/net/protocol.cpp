#include "net/protocol.h"

void Protocol::response_begin(Buffer& out, size_t* header)
{
    *header = out.data.size(); // save the header position
    buf_append_u32(out, 0);    // reserve 4 bytes
}

size_t Protocol::response_size(Buffer const& out, size_t header)
{
    return out.data.size() - header - 4;
}

void Protocol::response_end(Buffer& out, size_t header)
{
    size_t msg_size = response_size(out, header);

    if (msg_size > SMAX_MSG_LENGTH) {
        out.data.resize(header + 4);
        buf_out_err(out, TAG_ERR, ERR_TOO_BIG, "payload is too long.");
        msg_size = response_size(out, header);
    }

    uint32_t len = (uint32_t)msg_size;
    memcpy(&out.data[header], &len, 4); // write the real body size
}

int32_t Protocol::serialize_request(std::vector<std::string> const& command, Buffer& out)
{
    uint32_t total_command_size = 4;
    for (std::string const str : command) {
        total_command_size += 4 + str.size();
    }

    if (total_command_size > CMAX_MSG_LENGTH) {
        return -1;
    }

    buffer_append(out, (uint8_t const*)&total_command_size, 4);

    uint32_t n_str = command.size();
    buffer_append(out, (uint8_t const*)&n_str, 4);

    for (std::string const& str : command) {
        uint32_t str_size = (uint32_t)str.size();

        buffer_append(out, (uint8_t const*)&str_size, 4);
        buffer_append(out, (uint8_t const*)str.data(), str.size());
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

int32_t parse_response(uint8_t const* data, size_t size)
{
    if (size < 1) {
        Logger::alert("bad response");
        return -1;
    }

    switch (data[0]) {
    case TAG_NIL:
        return Parser::parse_nil(data, size);
    case TAG_ERR:
        return Parser::parse_err(data, size);
    case TAG_STR:
        return Parser::parse_str(data, size);
    case TAG_INT:
        return Parser::parse_int(data, size);
    case TAG_DBL:
        return Parser::parse_dbl(data, size);
    case TAG_ARR: {
        if (size < 1 + 4) {
            Logger::alert("bad response");
            return -1;
        }
        uint32_t len = 0;
        memcpy(&len, &data[1], 4);
        printf("(arr) len=%u\n", len);

        size_t arr_bytes = 1 + 4;
        for (uint32_t i = 0; i < len; ++i) {
            int32_t rv = parse_response(&data[arr_bytes], size - arr_bytes);
            if (rv < 0) {
                return rv;
            }
            arr_bytes += (size_t)rv;
        }

        printf("(arr) end\n");
        return (int32_t)arr_bytes;
    }
    default:
        Logger::alert("bad response");
        return -1;
    }
}

int32_t Protocol::deserialize_response(Buffer const& buffer)
{
    uint32_t size = 0;
    uint8_t const* data = buffer.data.data() + 4;
    memcpy(&size, buffer.data.data(), 4);

    if (size < 1) {
        Logger::alert("bad response");
        return -1;
    }

    return parse_response(data, size);
}
