#include "storage/zset.h"

// temporary struct for hashmap lookup
/// HKey mimics the shape of a hash node
struct HKey {
    HNode node;
    char const* name = nullptr;
    size_t len = 0;
};

bool hcmp(HNode* node, HNode* key)
{
    ZNode* znode = container_of(node, ZNode, hmap);
    HKey* hkey = container_of(key, HKey, node);
    if (znode->len != hkey->len) {
        return false;
    }
    return 0 == memcmp(znode->name, hkey->name, znode->len);
}

bool zless(AVLNode* lnode, char const* name, size_t len, double score)
{
    ZNode* zl = container_of(lnode, ZNode, tree);

    // compare by scores first, if equal -> compare by names
    if (zl->score != score) {
        return zl->score < score;
    }

    int ret_val = memcmp(zl->name, name, std::min(zl->len, len));

    return (ret_val != 0) ? (ret_val < 0) : (zl->len < len);
}

bool zless(AVLNode* lnode, AVLNode* rnode)
{
    ZNode* zr = container_of(rnode, ZNode, tree);
    return zless(lnode, zr->name, zr->len, zr->score);
}

// the allocation function
ZNode* znode_new(char const* name, size_t len, double score)
{
    ZNode* node = (ZNode*)malloc(sizeof(ZNode) + len);
    avl_init(&node->tree);

    node->hmap.hcode = str_hash((uint8_t const*)name, len);
    node->score = score;
    node->len = len;
    memcpy(&node->name[0], name, len);

    return node;
}

// the deallocation function
void znode_del(ZNode* node)
{
    if (!node) {
        return;
    }

    free(node);
}

// avl insert
void tree_insert(ZSet* zset, ZNode* node)
{
    AVLNode* parent = nullptr;
    AVLNode** from = &zset->root;

    while (*from) {
        parent = *from;
        from = zless(&node->tree, parent) ? &parent->left : &parent->right;
    }

    *from = &node->tree;
    (*from)->parent = parent;

    zset->root = avl_fix(*from);
}

void zset_update(ZSet* zset, ZNode* node, double score)
{
    zset->root = avl_del(&node->tree);
    avl_init(&node->tree);
    node->score = score;
    tree_insert(zset, node);
}

void tree_dispose(AVLNode* node)
{
    if (!node) {
        return;
    }

    tree_dispose(node->left);
    tree_dispose(node->right);

    znode_del(container_of(node, ZNode, tree));
}

// IMPLEMENTATIONS

bool zset_insert(ZSet* zset, char const* name, size_t len, double score)
{
    ZNode* node = zset_lookup(zset, name, len);

    if (node) {
        zset_update(zset, node, score);
        return false;
    }

    ZNode* new_node = znode_new(name, len, score);
    hm_insert(&zset->hmap, &new_node->hmap);
    tree_insert(zset, new_node);

    return true;
}

ZNode* zset_lookup(ZSet* zset, char const* name, size_t len)
{
    if (!zset->root) {
        return nullptr;
    }

    HKey key;

    key.node.hcode = str_hash((uint8_t const*)name, len);
    key.name = name;
    key.len = len;

    HNode* found = hm_lookup(&zset->hmap, &key.node, hcmp);

    return found ? container_of(found, ZNode, hmap) : nullptr;
}

void zset_delete(ZSet* zset, ZNode* node)
{
    if (!zset->root) {
        return;
    }

    HKey key;

    key.node.hcode = node->hmap.hcode;
    key.name = node->name;
    key.len = node->len;

    // delete from hashmap
    HNode* found = hm_delete(&zset->hmap, &key.node, hcmp);
    assert(found);

    // delete from avl
    zset->root = avl_del(&node->tree);
    znode_del(node); // free memory
}

/// seek: finding the lower bound
/// offset forward or backward by n positions.
/// iterate and output up to limit elements.
ZNode* zset_seekge(ZSet* zset, char const* name, size_t len, double score)
{
    AVLNode* found = nullptr;
    for (AVLNode* node = zset->root; node;) {
        if (zless(node, name, len, score)) // if current node < key then go right
        {
            node = node->right;
        } else { // current node >= key, candidate
            found = node;
            node = node->left; // search for even smaller valid candidate node
        }
    }

    return found ? container_of(found, ZNode, tree) : nullptr;
}

ZNode* znode_offset(ZNode* node, int64_t offset)
{
    AVLNode* tnode = node ? avl_offset(&node->tree, offset) : nullptr;
    return tnode ? container_of(tnode, ZNode, tree) : nullptr;
}

void zset_clear(ZSet* zset)
{
    hm_clear(&zset->hmap);
    tree_dispose(zset->root);

    zset->root = nullptr;
}
