#ifndef BUFFER_UTIL_H
#define BUFFER_UTIL_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace BufferUtil {
inline void buffer_append(std::vector<uint8_t>& buffer, uint8_t const* data, size_t len)
{
    // append to back
    buffer.insert(buffer.end(), data, data + len);
}

inline void buffer_consume(std::vector<uint8_t>& buffer, size_t len)
{
    // remove from front
    buffer.erase(buffer.begin(), buffer.begin() + len);
}

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
