#ifndef AVL_H
#define AVL_H

#include <cassert>
#include <cstdint>

struct AVLNode {
    AVLNode* left = nullptr;
    AVLNode* right = nullptr;
    AVLNode* parent = nullptr;

    uint32_t height = 0;
    uint32_t subtree_size = 0;
};

void avl_init(AVLNode* node);

uint32_t avl_height(AVLNode* node);
uint32_t avl_subtree_size(AVLNode* node);

AVLNode* avl_fix(AVLNode* node);
AVLNode* avl_del(AVLNode* node);

#endif
