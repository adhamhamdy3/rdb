#ifndef NETWORK_IO_H
#define NETWORK_IO_H

#include <cstdint>
#include <errno.h>
#include <unistd.h>

namespace NetworkIO {
inline int32_t read_n(int tcp_connection, uint8_t* rbuffer, size_t n)
{
    int32_t total_read = 0;

    while (total_read < (int32_t)n) {
        ssize_t ret_val = read(tcp_connection, rbuffer + total_read, n - total_read);

        if (ret_val > 0) {
            total_read += ret_val;
        } else if (ret_val == 0) { // EOF
            return total_read;
        } else { // error
            if (errno == EINTR)
                continue; // retry
            return -1;
        }
    }

    return total_read;
}

inline int32_t write_n(int tcp_connection, uint8_t const* wbuffer, size_t n)
{
    int32_t total_write = 0;

    while (total_write < (int32_t)n) {
        ssize_t ret_val = write(tcp_connection, wbuffer + total_write, n - total_write);
        if (ret_val > 0) { // error or EOF
            total_write += ret_val;
        } else if (ret_val == 0) {
            return total_write;
        } else {
            if (errno == EINTR)
                continue;
            return -1;
        }
    }

    return total_write;
}
} // namespace NetworkIO

#endif
