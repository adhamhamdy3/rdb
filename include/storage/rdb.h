#ifndef RDB_H
#define RDB_H

#include "hashtable.h"
#include "net/protocol.h"
#include "zset.h"
#include <string>
#include <vector>

// KV pair for the top-level hashtable
struct Entry {
    struct HNode node; // hashtable node
    std::string key;

    uint32_t type = T_INIT; // value type

    // one of the following at a time
    std::string value; // T_STR
    ZSet zset;         // T_ZSET
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

void do_zadd(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_zrem(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_zscore(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_zquery(std::vector<std::string> const& command, Buffer& buf, Database& db);

#endif
