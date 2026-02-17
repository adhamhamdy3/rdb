/*

#ifndef RDB_H
#define RDB_H

#include "hashtable.h"
#include <string>
#include <vector>

struct Database {
    HMap db; // top-level hashtable
};

// KV pair for the top-level hashtable
struct Entry {
    struct HNode node; // hashtable node
    std::string key;
    std::string value;
};

bool entry_eq(HNode* lnode, HNode* rnode);
uint64_t str_hash(uint8_t const* data, size_t len); // FNV hash function

void do_get(std::vector<std::string>& command, Response& response);
void do_set();

#endif


*/