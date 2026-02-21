#ifndef BUFFER_UTIL_H
#define BUFFER_UTIL_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace BufferUtil {

inline bool read_u32(uint8_t const*& cur, uint8_t const* end, uint32_t& out)
{
    if (cur + 4 > end) {
        return false;
    }

    memcpy(&out, cur, 4);
    cur += 4;

    return true;
}

inline bool read_str(uint8_t const*& cur, uint8_t const* end, uint32_t len, std::string& str)
{
    if (cur + len > end) {
        return false;
    }

    str.assign(cur, cur + len);
    cur += len;

    return true;
}

} // namespace BufferUtil

#endif
