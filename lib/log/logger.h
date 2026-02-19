#ifndef LOGGER_H
#define LOGGER_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

namespace Logger {

inline void alert(char const* msg)
{
    fprintf(stderr, "%s\n", msg);
}

inline void log_error(char const* msg)
{
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

} // namespace Logger

#endif
