#include "storage/core/heap.h"

size_t heap_parent(size_t i)
{
    return (i + 1) / 2 - 1;
}

size_t heap_left(size_t i)
{
    return i * 2 + 1;
}

size_t heap_right(size_t i)
{
    return i * 2 + 2;
}

void heap_up(HeapItem* heap_arr, size_t pos)
{
    HeapItem temp = heap_arr[pos];

    while (pos > 0 && heap_arr[heap_parent(pos)].value > temp.value) {
        heap_arr[pos] = heap_arr[heap_parent(pos)]; // swap parent with child
        *heap_arr[pos].ref = pos;                   // update parent position
        pos = heap_parent(pos);                     // go upward
    }

    heap_arr[pos] = temp;     // place the new item in the right position
    *heap_arr[pos].ref = pos; // update its position
}

void heap_down(HeapItem* heap_arr, size_t pos, size_t len)
{
    HeapItem temp = heap_arr[pos];

    while (true) {
        size_t l = heap_left(pos);
        size_t r = heap_right(pos);
        size_t min_pos = pos;

        uint64_t min_val = temp.value;

        if (l < len && heap_arr[l].value < min_val) {
            min_pos = l;
            min_val = heap_arr[l].value;
        }

        if (r < len && heap_arr[r].value < min_val) {
            min_pos = r;
            min_val = heap_arr[r].value;
        }

        if (min_pos == pos) {
            break; // already in the right pos
        }

        // swap with the child
        heap_arr[pos] = heap_arr[min_pos];
        *heap_arr[pos].ref = pos;

        pos = min_pos; // go downward
    }

    heap_arr[pos] = temp;
    *heap_arr[pos].ref = pos;
}

void heap_update(HeapItem* heap_arr, size_t pos, size_t len)
{
    if (pos < len && heap_arr[heap_parent(pos)].value > heap_arr[pos].value) {
        heap_up(heap_arr, pos);
    } else {
        heap_down(heap_arr, pos, len);
    }
}

void heap_insert(Heap& heap, uint64_t val, size_t* pos)
{
    HeapItem item = { val, pos };

    if (*pos < heap.heap_arr.size()) { // already exist, update
        heap.heap_arr[*pos] = item;
    } else {
        *pos = heap.heap_arr.size();
        heap.heap_arr.push_back(item);
    }

    heap_update(heap.heap_arr.data(), *pos, heap.heap_arr.size());
}

void heap_delete(Heap& heap, size_t pos)
{
    heap.heap_arr[pos] = heap.heap_arr.back();
    heap.heap_arr.pop_back();

    if (pos < heap.heap_arr.size()) {
        heap_update(heap.heap_arr.data(), pos, heap.heap_arr.size());
    }
}
