#ifndef PROTOCOL_H
#define PROTOCOL_H

#define RES_OK 0
#define RES_ERR 1
#define RES_NX 2

#include <cstdint>
#include <vector>

size_t const MAX_MSG_LENGTH = (32 << 20);
size_t const MAX_ARGS = 200 * 1000;

struct Response {
    uint32_t status = 0;
    std::vector<uint8_t> data;
};

#endif
