#include "storage/core/list.h"

void dlist_init(DList* node)
{
    node->prev = node->next = node;
}

bool dlist_isempty(DList* node)
{
    return node->next == node;
}

void dlist_detach(DList* node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

void dlist_insert_before(DList* target, DList* node)
{
    node->prev = target->prev;
    node->next = target;

    target->prev->next = node;
    target->prev = node;
}
