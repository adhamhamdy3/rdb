#ifndef RDB_H
#define RDB_H

#include "core/hashtable.h"
#include "core/heap.h"
#include "entry.h"
#include "storage/core/list.h"
#include "timer.h"
#include <vector>

struct Database {
    HMap hashmap; // top-level hashtable

    // timers for idle connections
    DList idle_queue;

    // timers for TTLs
    Heap heap;
};

void do_get(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_set(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_del(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_keys(std::vector<std::string> const& command, Buffer& buf, Database& db);

void do_zadd(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_zrem(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_zscore(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_zquery(std::vector<std::string> const& command, Buffer& buf, Database& db);

void do_expire(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_ttl(std::vector<std::string> const& command, Buffer& buf, Database& db);

#endif
