#ifndef RDB_H
#define RDB_H

#include "hashtable.h"
#include "heap.h"
#include "net/protocol.h"
#include "zset.h"
#include <string>
#include <vector>

// TODO: migrate to separate header
// KV pair for the top-level hashtable
struct Entry {
    struct HNode node; // hashtable node
    std::string key;

    uint32_t type = T_INIT; // value type

    // one of the following at a time
    std::string value; // T_STR
    ZSet zset;         // T_ZSET

    size_t heap_idx = -1; // entry's position in the heap array
};

Entry* entry_new(uint32_t type);
void entry_del(Entry* entry);
bool entry_eq(HNode* lnode, HNode* rnode);

struct LookupKey {
    HNode node;
    std::string key;
};

bool lk_entry_eq(HNode* lnode, HNode* key);

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
