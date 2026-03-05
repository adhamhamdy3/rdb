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

    // TODO: entry_set_ttl(entry, -1); // remove from heap

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

// FIXME:
bool cb_keys(HNode* node, void* arg)
{
    Buffer& out = *(Buffer*)arg;
    std::string const& key = container_of(node, Entry, node)->key;
    buf_out_str(out, TAG_STR, key.data(), key.size());
    return true;
}

ZSet* expect_zset(std::string const& key, HMap* hmap)
{
    LookupKey lookup;

    lookup.key = key;
    lookup.node.hcode = str_hash((uint8_t const*)key.data(), key.size());

    HNode* hnode = hm_lookup(hmap, &lookup.node, lk_entry_eq);
    if (!hnode) {
        return nullptr;
    }

    Entry* entry = container_of(hnode, Entry, node);

    return entry->type == T_ZSET ? &entry->zset : nullptr;
}

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

// FIXME:
void do_keys(std::vector<std::string> const& command, Buffer& buf, Database& db)
{
    buf_out_arr(buf, TAG_ARR, (uint32_t)hm_size(&db.hashmap));
    hm_foreach(&db.hashmap, &cb_keys, (void*)&buf);
}

// zadd zset score name
void do_zadd(std::vector<std::string> const& command, Buffer& buf, Database& db)
{
    double score = 0;
    if (!BufferUtil::str_to_dbl(command[2], score)) {
        return buf_out_err(buf, TAG_ERR, ERR_BAD_ARG, "expected floating point number");
    }

    LookupKey lk;

    lk.key = command[1];
    lk.node.hcode = str_hash((uint8_t const*)lk.key.data(), lk.key.size());

    HNode* hnode = hm_lookup(&db.hashmap, &lk.node, lk_entry_eq);

    Entry* entry = nullptr;
    if (!hnode) { // does not exist yet, create a new ZSet entry
        entry = entry_new(T_ZSET);

        entry->key = lk.key;
        entry->node.hcode = lk.node.hcode;

        hm_insert(&db.hashmap, &entry->node);
    } else { // already exists
        entry = container_of(hnode, Entry, node);
        if (entry->type != T_ZSET) {
            return buf_out_err(buf, TAG_ERR, ERR_BAD_TYP, "expect zset");
        }
    }

    std::string const& name = command[3];
    bool done = zset_insert(&entry->zset, name.data(), name.size(), score);

    return buf_out_int(buf, TAG_INT, (int64_t)done);
}

// zrem zset name
void do_zrem(std::vector<std::string> const& command, Buffer& buf, Database& db)
{
    ZSet* zset = expect_zset(command[1], &db.hashmap);
    if (!zset) {
        return buf_out_err(buf, TAG_ERR, ERR_BAD_TYP, "zset key is not exist");
    }

    std::string const& name = command[2];
    ZNode* znode = zset_lookup(zset, name.data(), name.size());

    if (znode) {
        zset_delete(zset, znode);
    }

    return buf_out_int(buf, TAG_INT, znode ? 1 : 0);
}

// zscore zset name
void do_zscore(std::vector<std::string> const& command, Buffer& buf, Database& db)
{
    ZSet* zset = expect_zset(command[1], &db.hashmap);
    if (!zset) {
        return buf_out_err(buf, TAG_ERR, ERR_BAD_TYP, "zset key is not exist");
    }

    std::string const& name = command[2];
    ZNode* znode = zset_lookup(zset, name.data(), name.size());

    return znode ? buf_out_dbl(buf, TAG_DBL, znode->score) : buf_out_nil(buf, TAG_NIL);
}

// zquery zset score name offset limit
void do_zquery(std::vector<std::string> const& command, Buffer& buf, Database& db)
{
    double score = 0;
    if (!BufferUtil::str_to_dbl(command[2], score)) {
        return buf_out_err(buf, TAG_ERR, ERR_BAD_ARG, "expected floating point number");
    }

    std::string name = command[3];
    int64_t offset = 0, limit = 0;
    if (!BufferUtil::str_to_int(command[4], offset) || !BufferUtil::str_to_int(command[5], limit)) {
        return buf_out_err(buf, TAG_ERR, ERR_BAD_ARG, "expected integer number");
    }

    // get the zset
    ZSet* zset = expect_zset(command[1], &db.hashmap);
    if (!zset) {
        return buf_out_err(buf, TAG_ERR, ERR_BAD_TYP, "expect zset");
    }

    // seek to the key
    if (limit <= 0) {
        return buf_out_arr(buf, TAG_ARR, 0);
    }

    ZNode* znode = zset_seekge(zset, name.data(), name.size(), score);

    // iterate and fill the buffer
    int64_t n = 0;
    size_t ctx = buf_begin_arr(buf, TAG_ARR);

    while (znode && n < limit) {
        buf_out_str(buf, TAG_STR, znode->name, znode->len);
        buf_out_dbl(buf, TAG_DBL, znode->score);

        znode = znode_offset(znode, +1);

        n += 2;
    }

    buf_out_arr(buf, TAG_ARR, (uint32_t)n);
}
