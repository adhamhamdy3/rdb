#include "storage/rdb.h"

bool entry_eq(HNode* lnode, HNode* rnode)
{
    struct Entry* l = container_of(lnode, struct Entry, node);
    struct Entry* r = container_of(rnode, struct Entry, node);

    return l->key == r->key;
}

uint64_t str_hash(uint8_t const* data, size_t len) // FNV hash function
{
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++) {
        h = (h + data[i]) * 0x01000193;
    }
    return h;
}

void do_get(std::vector<std::string> const& command, Response& response, Database& db)
{
    Entry entry;
    entry.key = command[1];
    entry.node.hcode = str_hash((uint8_t const*)entry.key.data(), entry.key.size());

    HNode* node = hm_lookup(&db.hashmap, &entry.node, entry_eq);
    if (!node) {
        response.status = RES_NX;
        return;
    }

    std::string const& value = container_of(node, Entry, node)->value;
    assert(value.size() < SMAX_MSG_LENGTH);

    response.data.assign(value.begin(), value.end());
}

void do_set(std::vector<std::string> const& command, Response& response, Database& db)
{
    Entry entry;
    entry.key = command[1];
    entry.node.hcode = str_hash((uint8_t const*)entry.key.data(), entry.key.size());

    HNode* node = hm_lookup(&db.hashmap, &entry.node, entry_eq);

    if (node) { // already exist in the database, update the value
        container_of(node, Entry, node)->value = command[2];
    } else { // not found, insert new kv pair
        Entry* e = new Entry();

        e->key = entry.key;
        e->node.hcode = entry.node.hcode;
        e->value = command[2];

        hm_insert(&db.hashmap, &e->node);
    }
}

void do_del(std::vector<std::string> const& command, Response& response, Database& db)
{
    Entry entry;
    entry.key = command[1];
    entry.node.hcode = str_hash((uint8_t const*)entry.key.data(), entry.key.size());

    HNode* node = hm_delete(&db.hashmap, &entry.node, entry_eq);

    if (node) {
        delete container_of(node, Entry, node);
    }
}
