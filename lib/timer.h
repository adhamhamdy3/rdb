#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <time.h>

/*
 struct timespec members:
    time_t tv_sec: Whole seconds (typically seconds since the Epoch).
    long tv_nsec: Nanoseconds (0 to 999'999'999).
*/

namespace Timer {

inline uint64_t get_monotonic_msec()
{
    struct timespec tv = { 0, 0 };
    clock_gettime(CLOCK_MONOTONIC, &tv);

    return uint64_t(tv.tv_sec) * 1000 + tv.tv_nsec / 1000 / 1000;
}
} // namespace Timer

#endif
