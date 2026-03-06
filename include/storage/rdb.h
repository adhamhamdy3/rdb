#ifndef RDB_H
#define RDB_H

#include "entry.h"
#include "hashtable.h"
#include "heap.h"
#include <vector>

struct Database {
    HMap hashmap; // top-level hashtable
};

void do_get(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_set(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_del(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_keys(std::vector<std::string> const& command, Buffer& buf, Database& db);

void do_zadd(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_zrem(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_zscore(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_zquery(std::vector<std::string> const& command, Buffer& buf, Database& db);

// TODO: do_expire(std::vector<std::string> const& command, Buffer& buf, Database& db);
// TODO: do_ttl(std::vector<std::string> const& command, Buffer& buf, Database& db);

#endif
