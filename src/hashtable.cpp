#include "hashtable.h"
#include <assert.h>

/// (n % 2) - bitwise AND modulo
#define mod2(n) ((n - 1) & n)

size_t const MAX_LOAD_FACTOR = 8;
size_t const MAX_REHASHING_WORK = 128;

void h_init(HTable* htable, size_t n)
{
    assert(n > 0 && mod2(n) == 0); // n must be power of 2
    htable->table = (HNode**)calloc(n, sizeof(HNode*));
    htable->mask = n - 1;
    htable->size = 0;
}

void hm_rehashing(HMap* hmap)
{
    hmap->older = hmap->newer;
    h_init(&hmap->newer, (hmap->newer.mask + 1) * 2);
    hmap->migrate_pos = 0;
}

void hm_migrate_some(HMap* hmap)
{
    // one call will move at most 128 nodes
    size_t work_count = 0;
    while (work_count < MAX_REHASHING_WORK && hmap->older.size > 0) {
        HNode** from = &hmap->older.table[hmap->migrate_pos];
        if (!*from) {
            hmap->migrate_pos++; // empty slot
            continue;
        }

        h_insert(&hmap->newer, h_detach(&hmap->older, from));
        work_count++;
    }

    if (hmap->older.size == 0 && hmap->older.table) {
        free(hmap->older.table);
        hmap->older = HTable {};
    }
}

/// IMPLEMENTATIONS

void h_insert(HTable* htable, HNode* node)
{
    size_t pos = node->hcode & htable->mask;
    HNode* next = htable->table[pos];
    node->next = next;
    htable->table[pos] = node;
    htable->size++;
}

HNode** h_lookup(HTable* htable, HNode* key, bool (*eq)(HNode*, HNode*))
{
    if (!htable->table) {
        return nullptr;
    }

    size_t pos = key->hcode & htable->mask;
    HNode** from = &htable->table[pos];

    for (HNode* cur; (cur = *from) != nullptr; from = &cur->next) {
        if (cur->hcode == key->hcode && eq(cur, key)) {
            return from;
        }
    }

    return nullptr;
}

HNode* h_detach(HTable* htable, HNode** from)
{
    HNode* node = *from;
    *from = node->next;
    htable->size--;

    return node;
}

void hm_insert(HMap* hmap, HNode* node)
{
    if (!hmap->newer.table) {
        h_init(&hmap->newer, 4); // init with 4 slots if empty
    }

    h_insert(&hmap->newer, node);

    if (!hmap->older.table) {
        size_t threshold = (hmap->newer.mask + 1) * MAX_LOAD_FACTOR;
        if (hmap->newer.size >= threshold) {
            hm_rehashing(hmap);
        }
    }

    hm_migrate_some(hmap);
}

HNode* hm_lookup(HMap* hmap, HNode* key, bool (*eq)(HNode*, HNode*))
{
    hm_migrate_some(hmap);

    HNode** from = h_lookup(&hmap->newer, key, eq);

    // if not found in newer hash map, perform lookup in the older one
    if (!from) {
        from = h_lookup(&hmap->older, key, eq);
    }

    return from ? *from : nullptr;
}

HNode* hm_delete(HMap* hmap, HNode* key, bool (*eq)(HNode*, HNode*))
{
    hm_migrate_some(hmap);

    HNode** from = h_lookup(&hmap->newer, key, eq);
    if (from) {
        return h_detach(&hmap->newer, from);
    }

    from = h_lookup(&hmap->older, key, eq);
    if (from) {
        return h_detach(&hmap->older, from);
    }

    return nullptr;
}

void hm_clear(HMap* hmap)
{
    free(hmap->newer.table);
    free(hmap->older.table);
    *hmap = HMap {};
}

size_t hm_size(HMap* hmap)
{
    return hmap->newer.size + hmap->older.size;
}
