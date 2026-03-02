#include "storage/rdb.h"

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

bool entry_eq(HNode* lnode, HNode* rnode)
{
    struct Entry* l = container_of(lnode, struct Entry, node);
    struct Entry* r = container_of(rnode, struct Entry, node);

    return l->key == r->key;
}

bool lk_entry_eq(HNode* node, HNode* key)
{
    Entry* ent = container_of(node, Entry, node);
    LookupKey* keydata = container_of(key, LookupKey, node);

    return ent->key == keydata->key;
}

// TODO: use LookupKey instead of Entry
void do_get(std::vector<std::string> const& command, Buffer& buf, Database& db)
{
    LookupKey lk;
    lk.key = command[1];
    lk.node.hcode = str_hash((uint8_t const*)lk.key.data(), lk.key.size());

    HNode* node = hm_lookup(&db.hashmap, &lk.node, lk_entry_eq);
    if (!node) {
        return buf_out_nil(buf, TAG_NIL);
    }

    Entry* entry = container_of(node, Entry, node);
    if (entry->type != T_STR) {
        return buf_out_err(buf, TAG_ERR, ERR_BAD_TYP, "not a string value");
    }

    // copy the value
    return buf_out_str(buf, TAG_STR, entry->value.data(), entry->value.size());
}

void do_set(std::vector<std::string> const& command, Buffer& buf, Database& db)
{
    LookupKey lk;
    lk.key = command[1];
    lk.node.hcode = str_hash((uint8_t const*)lk.key.data(), lk.key.size());

    HNode* node = hm_lookup(&db.hashmap, &lk.node, lk_entry_eq);

    if (node) { // already exist in the database, update the value
        Entry* entry = container_of(node, Entry, node);
        if (entry->type != T_STR) {
            return buf_out_err(buf, TAG_ERR, ERR_BAD_TYP, "a non-string value already exists");
        }
        entry->value = command[2];
    } else { // not found, insert new kv pair
        Entry* e = entry_new(TAG_STR);

        e->key = lk.key;
        e->node.hcode = lk.node.hcode;
        e->value = command[2];

        hm_insert(&db.hashmap, &e->node);
    }

    return buf_out_nil(buf, TAG_NIL);
}

void do_del(std::vector<std::string> const& command, Buffer& buf, Database& db)
{
    LookupKey lk;
    lk.key = command[1];
    lk.node.hcode = str_hash((uint8_t const*)lk.key.data(), lk.key.size());

    HNode* node = hm_delete(&db.hashmap, &lk.node, lk_entry_eq);

    if (node) {
        delete container_of(node, Entry, node);
    }

    return buf_out_int(buf, TAG_INT, node ? 1 : 0);
}