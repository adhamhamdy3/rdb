#ifndef TIMERS_H
#define TIMERS_H

#include "net/server.h"
#include <cstdint>

// forward declarations
struct Server;
struct connection_state;
struct Entry;

uint64_t const MAX_IDLE_TIMEOUT = 5000; // 5 sec

int32_t next_timer_ms(Server& server);
void reset_timers(Server& server, connection_state* conn);
void process_timers(Server& server);
void entry_set_ttl(Server& server, Entry* entry, int64_t ttl_ms);

#endif
