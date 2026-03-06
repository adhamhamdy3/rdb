#ifndef PARSER_H
#define PARSER_H

#include "log/logger.h"
#include <cstdint>
#include <stdio.h>
#include <string.h>

namespace Parser {

inline int32_t parse_nil(uint8_t const* data, uint32_t size)
{
    printf("(nil)\n");
    return 1;
}

inline int32_t parse_err(uint8_t const* data, uint32_t size)
{
    if (size < 1 + 8) {
        fprintf(stderr, "%s\n", "bad response");
        return -1;
    }
    int32_t code = 0;
    uint32_t len = 0;
    memcpy(&code, data + 1, 4);
    memcpy(&len, data + 1 + 4, 4);
    if (size < 1 + 8 + len) {
        fprintf(stderr, "%s\n", "bad response");
        return -1;
    }
    printf("(err) %d %.*s\n", code, len, data + 1 + 8);
    return 1 + 8 + len;
}

inline int32_t parse_str(uint8_t const* data, uint32_t size)
{
    if (size < 1 + 4) {
        fprintf(stderr, "%s\n", "bad response");
        return -1;
    }
    uint32_t len = 0;
    memcpy(&len, data + 1, 4);
    if (size < 1 + 4 + len) {
        fprintf(stderr, "%s\n", "bad response");
        return -1;
    }
    printf("(str) %.*s\n", len, data + 1 + 4);
    return 1 + 4 + len;
}

inline int32_t parse_int(uint8_t const* data, uint32_t size)
{
    if (size < 1 + 8) {
        fprintf(stderr, "%s\n", "bad response");
        return -1;
    }
    int64_t val = 0;
    memcpy(&val, data + 1, 8);
    printf("(int) %ld\n", val);
    return 1 + 8;
}

inline int32_t parse_dbl(uint8_t const* data, uint32_t size)
{
    if (size < 1 + 8) {
        fprintf(stderr, "%s\n", "bad response");
        return -1;
    }
    double val = 0;
    memcpy(&val, data + 1, 8);
    printf("(dbl) %g\n", val);
    return 1 + 8;
}

} // namespace Parser

#endif
