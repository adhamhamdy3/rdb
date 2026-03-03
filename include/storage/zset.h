#ifndef ZSET_H
#define ZSET_H

#include "avl.h"
#include "hashtable.h"
#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <string>

// value types
#define T_INIT 0
#define T_STR 1
#define T_ZSET 2

/// in ZNode (intrusive data structure)
/// both point into the same ZNode objects - no duplication
struct ZNode {
    AVLNode tree;
    HNode hmap;
    double score = 0;
    size_t len = 0;

    /// flexible array instead of 'char* name' for one allocation/free syscalls for the whole struct
    /// instead of one for the struct and one for the 'char* name', also for the contiguous allocation layout
    /// the string data will live immediately after this struct in memory
    char name[];
};

struct ZSet {
    AVLNode* root = nullptr; // sorted avl set for indexing by (score, name)
    HMap hmap;               // hashmap indexed by name
};

bool zset_insert(ZSet* zset, char const* name, size_t len, double score);
ZNode* zset_lookup(ZSet* zset, char const* name, size_t len);
void zset_delete(ZSet* zset, ZNode* node);
ZNode* zset_seekge(ZSet* zset, char const* name, size_t len, double score);
ZNode* znode_offset(ZNode* node, int64_t offset);
void zset_clear(ZSet* zset);

#endif
