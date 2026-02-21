#ifndef BUFFER_H
#define BUFFER_H

#include <cstdint>
#include <string>
#include <vector>

struct Buffer {
    std::vector<uint8_t> data;
};

void buffer_append(Buffer& buffer, uint8_t const* data, size_t len);
void buffer_consume(Buffer& buffer, size_t len);

// cast the address of the value to (const uint8_t*),
// reinterpreting its raw memory as individual bytes, then copy N bytes into
// the buffer - same memory, different perspective (uint32_t* vs uint8_t*)
void buf_append_u8(Buffer& buf, uint8_t data);   // 1 byte
void buf_append_u32(Buffer& buf, uint32_t data); // 4 bytes
void buf_append_i64(Buffer& buf, int64_t data);  // 8 bytes
void buf_append_dbl(Buffer& buf, double data);   // 8 bytes

void buf_out_nil(Buffer& buf, uint8_t tag);
void buf_out_str(Buffer& buf, uint8_t tag, char const* s, size_t size);
void buf_out_int(Buffer& buf, uint8_t tag, int64_t value);
void buf_out_arr(Buffer& buf, uint8_t tag, uint32_t n);
void buf_out_err(Buffer& buf, uint8_t tag, uint32_t code, std::string const& msg);

#endif
