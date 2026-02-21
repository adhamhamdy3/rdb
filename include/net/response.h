#ifndef RESPONSE_H
#define RESPONSE_H

#define RES_OK 0
#define RES_ERR 1
#define RES_NX 2

#include <cstdint>
#include <vector>

struct Response {
    uint32_t status = 0;
    std::vector<uint8_t> data;
};

#endif
