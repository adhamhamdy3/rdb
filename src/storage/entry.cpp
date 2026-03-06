#include "storage/entry.h"

Entry* entry_new(uint32_t type)
{
    Entry* entry = new Entry();
    entry->type = type;
    return entry;
}

void entry_del(Entry* entry)
{
    if (entry->type == T_ZSET) {
        zset_clear(&entry->zset);
    }

    delete entry;
}

bool entry_eq(HNode* node, HNode* lkey)
{
    struct Entry* entry = container_of(node, struct Entry, node);
    struct LookupKey* key = container_of(lkey, struct LookupKey, node);
    return entry->key == key->key;
}
