#ifndef RDB_H
#define RDB_H

#include "hashtable.h"
#include "net/protocol.h"
#include <string>
#include <vector>

// KV pair for the top-level hashtable
struct Entry {
    struct HNode node; // hashtable node
    std::string key;
    std::string value;
};

bool entry_eq(HNode* lnode, HNode* rnode);

struct Database {
    HMap hashmap; // top-level hashtable
};

void do_get(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_set(std::vector<std::string> const& command, Buffer& buf, Database& db);
void do_del(std::vector<std::string> const& command, Buffer& buf, Database& db);

#endif
