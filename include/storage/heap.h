#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>
#include <vector>

struct HeapItem {
    uint64_t value = 0;
    size_t* ref = nullptr; // current index of this item inside the heap array
};

struct Heap {
    std::vector<HeapItem> heap_arr;
};

void heap_insert(Heap& heap, uint64_t val, size_t* pos);
void heap_delete(Heap& heap, size_t pos);

#endif
