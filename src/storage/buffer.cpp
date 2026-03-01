#include "storage/buffer.h"

void buffer_append(Buffer& buf, uint8_t const* data, size_t len)
{
    // append to back
    buf.data.insert(buf.data.end(), data, data + len);
}

void buffer_consume(Buffer& buf, size_t len)
{
    // remove from front
    buf.data.erase(buf.data.begin(), buf.data.begin() + len);
}

void buf_append_u8(Buffer& buf, uint8_t data)
{
    buf.data.push_back(data);
}

void buf_append_u32(Buffer& buf, uint32_t data)
{
    buffer_append(buf, (uint8_t const*)&data, 4);
}

void buf_append_i64(Buffer& buf, int64_t data)
{
    buffer_append(buf, (uint8_t const*)&data, 8);
}

void buf_append_dbl(Buffer& buf, double data)
{
    buffer_append(buf, (uint8_t const*)&data, 8);
}

void buf_out_nil(Buffer& buf, uint8_t tag)
{
    buf_append_u8(buf, tag);
}

void buf_out_str(Buffer& buf, uint8_t tag, char const* s, size_t size)
{
    buf_append_u8(buf, tag);
    buf_append_u32(buf, (uint32_t)size);
    buffer_append(buf, (uint8_t const*)s, size);
}

void buf_out_int(Buffer& buf, uint8_t tag, int64_t value)
{
    buf_append_u8(buf, tag);
    buf_append_i64(buf, value);
}

void buf_out_dbl(Buffer& buf, uint8_t tag, double val)
{
    buf_append_u8(buf, tag);
    buf_append_dbl(buf, val);
}

size_t buf_begin_arr(Buffer& buf, uint8_t tag)
{
    buf.data.push_back(tag);
    buf_append_u32(buf, 0);
    return buf.data.size() - 4; // ctx
}

void buf_out_arr(Buffer& buf, uint8_t tag, uint32_t n)
{
    buf_append_u8(buf, tag);
    buf_append_u32(buf, n);
}

void buf_end_arr(Buffer& buf, uint8_t tag, size_t ctx, uint32_t n)
{
    assert(buf.data[ctx - 1] == tag);
    memcpy(&buf.data[ctx], &n, 4);
}

void buf_out_err(Buffer& buf, uint8_t tag, uint32_t code, std::string const& msg)
{
    buf_append_u8(buf, tag);
    buf_append_u32(buf, code);
    buf_append_u32(buf, msg.size());
    buffer_append(buf, (uint8_t const*)msg.data(), msg.size());
}
