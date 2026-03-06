#ifndef ENTRY_H
#define ENTRY_H

#include "net/protocol.h"
#include "zset.h"
#include <string>

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

#endif
