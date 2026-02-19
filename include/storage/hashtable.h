#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#define container_of(ptr, T, member) \
    ((T*)((char*)ptr - offsetof(T, member)))

struct HNode {
    HNode* next = nullptr;
    uint64_t hcode = 0; // hash value of the key
};

struct HTable {
    HNode** table = nullptr; // array of slots (each slot is a list head)
    size_t mask = 0;         // power of 2 array size, 2^n - 1 for faster hashing (bitwise AND modulo)
    size_t size = 0;         // table size
};

void h_insert(HTable* htable, HNode* node);
HNode** h_lookup(HTable* htable, HNode* key, bool (*eq)(HNode*, HNode*));
HNode* h_detach(HTable* htable, HNode** from);

struct HMap {
    HTable older;
    HTable newer;
    size_t migrate_pos = 0;
};

void hm_insert(HMap* hmap, HNode* node);
HNode* hm_lookup(HMap* hmap, HNode* key, bool (*eq)(HNode*, HNode*));
HNode* hm_delete(HMap* hmap, HNode* key, bool (*eq)(HNode*, HNode*));
void hm_clear(HMap* hmap);
size_t hm_size(HMap* hmap);

#endif
