#include "storage/avl.h"

uint32_t avl_height(AVLNode* node) { return node ? node->height : 0; }

uint32_t avl_subtree_size(AVLNode* node) { return node ? node->subtree_size : 0; }

void avl_init(AVLNode* node)
{
    node->left = node->right = node->parent = nullptr;
    node->height = 1;
    node->subtree_size = 1;
}

inline uint32_t max(uint32_t lhs, uint32_t rhs) { return lhs > rhs ? lhs : rhs; }

inline void avl_update(AVLNode* node)
{
    node->height = 1 + max(avl_height(node->left), avl_height(node->right));
    node->subtree_size = 1 + avl_subtree_size(node->left) + avl_subtree_size(node->right);
}

inline AVLNode* rotate_left(AVLNode* node)
{
    AVLNode* parent = node->parent;
    AVLNode* new_parent = node->right;
    AVLNode* inner_node = new_parent->left;

    node->right = inner_node;
    if (inner_node) {
        inner_node->parent = node;
    }

    // NOTE: the parent-to-child link is not updated here.
    new_parent->parent = parent;
    new_parent->left = node;
    node->parent = new_parent;

    avl_update(node);
    avl_update(new_parent);

    return new_parent;
}

inline AVLNode* rotate_right(AVLNode* node)
{
    AVLNode* parent = node->parent;
    AVLNode* new_parent = node->left;
    AVLNode* inner_node = new_parent->right;

    node->left = inner_node;
    if (inner_node) {
        inner_node->parent = node;
    }

    // NOTE: the parent-to-child link is not updated here.
    new_parent->parent = parent;
    new_parent->right = node;
    node->parent = new_parent;

    avl_update(node);
    avl_update(new_parent);

    return new_parent;
}

inline AVLNode* avl_fix_left(AVLNode* node)
{
    if (avl_height(node->left->left) < avl_height(node->left->right)) {
        node->left = rotate_left(node->left); // T2
    }

    return rotate_right(node); // T1
}

inline AVLNode* avl_fix_right(AVLNode* node)
{
    if (avl_height(node->right->right) < avl_height(node->right->left)) {
        node->right = rotate_right(node->right); // T2
    }

    return rotate_left(node); // T1
}

AVLNode* avl_fix(AVLNode* node)
{
    while (true) {
        AVLNode** from = &node; // save the fixed subtree here
        AVLNode* parent = node->parent;

        if (parent) {
            // attach the fixed subtree to the parent
            from = parent->left == node ? &parent->left : &parent->right;
        } // else, save to the local variable (node)

        avl_update(node);

        uint32_t l = avl_height(node->left);
        uint32_t r = avl_height(node->right);

        if (l == r + 2) {
            *from = avl_fix_left(node);
        } else if (l + 2 == r) {
            *from = avl_fix_right(node);
        }

        // root node, stop
        if (!parent) {
            return *from;
        }

        // move upward
        node = parent;
    }
}

AVLNode* avl_del_easy(AVLNode* node)
{
    assert(!node->left || !node->right);

    AVLNode* parent = node->parent;
    AVLNode* child = node->left ? node->left : node->right;

    // update the child's parent pointer
    if (child) {
        child->parent = parent; // can be nullptr
    }

    // attach the child to the grandparent
    if (!parent) {
        return child; // removing the root node
    }

    AVLNode** from = parent->left == node ? &parent->left : &parent->right;
    *from = child;

    // rebalance the updated tree
    return avl_fix(parent);
}

AVLNode* avl_del(AVLNode* node)
{
    if (!node->left || !node->right) // has only one child
    {
        return avl_del_easy(node);
    }

    AVLNode* victim = node->right;
    while (victim->left) {
        victim = victim->left;
    }

    // detach the victim
    AVLNode* root = avl_del_easy(victim);

    *victim = *node;

    if (victim->left) {
        victim->left->parent = victim;
    }

    AVLNode** from = &root;
    AVLNode* parent = node->parent;
    if (parent) {
        from = parent->left == node ? &parent->left : &parent->right;
    }

    *from = victim;

    return root;
}

AVLNode* avl_offset(AVLNode* node, int64_t offset)
{
    int64_t pos = 0;

    while (pos != offset) {
        if (pos < offset && pos + avl_subtree_size(node) >= offset) {
            node = node->right;
            pos += avl_subtree_size(node->left) + 1;
        } else if (pos > offset && pos - avl_subtree_size(node) <= offset) {
            node = node->left;
            pos -= avl_subtree_size(node->right) + 1;
        } else {
            AVLNode* parent = node->parent;
            if (!parent)
                return nullptr;

            if (node == parent->left) {
                pos += avl_subtree_size(node->right) + 1;
            } else {
                pos -= avl_subtree_size(node->left) + 1;
            }
        }
    }

    return node;
}
