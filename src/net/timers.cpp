#include "net/timers.h"
#include "net/server.h"

int32_t next_timer_ms(Server& server)
{
    uint64_t now_ms = Timer::get_monotonic_msec();
    uint64_t next_ms = (uint64_t)-1; // infinity

    // check idle connections
    if (!dlist_isempty(&server.db.idle_queue)) {
        connection_state* conn = container_of(server.db.idle_queue.next, connection_state, idle_node);
        next_ms = conn->last_active_ms + MAX_IDLE_TIMEOUT;
    }

    // check TTL timers
    if (!server.db.heap.heap_arr.empty() && server.db.heap.heap_arr[0].value < next_ms) {
        next_ms = server.db.heap.heap_arr[0].value;
    }

    if (next_ms == (uint64_t)-1) {
        return -1; // no timers, wait
    }

    if (next_ms <= now_ms) {
        return 0;
    }

    return int32_t(next_ms - now_ms);
}

void reset_timer(Server& server, connection_state* conn)
{
    dlist_detach(&conn->idle_node);
    conn->last_active_ms = Timer::get_monotonic_msec();
    dlist_insert_before(&server.db.idle_queue, &conn->idle_node);
}

void process_timers(Server& server)
{
    // idle timers
    uint64_t now_ms = Timer::get_monotonic_msec();
    while (!dlist_isempty(&server.db.idle_queue)) {
        connection_state* conn = container_of(server.db.idle_queue.next, connection_state, idle_node);
        int32_t next_ms = conn->last_active_ms + MAX_IDLE_TIMEOUT;

        if (next_ms >= now_ms) {
            break;
        }

        fprintf(stderr, "closing idle connection: %d\n", conn->tcp_socket);
        conn_destroy(server, conn);
    }

    // TTL timers
    size_t const max_work = 2000;
    size_t works = 0;

    auto const& heap_arr = server.db.heap.heap_arr;
    while (!heap_arr.empty() && heap_arr[0].value < now_ms && works++ < max_work) {
        Entry* entry = container_of(heap_arr[0].ref, Entry, heap_idx);

        HNode* hnode = hm_delete(&server.db.hashmap, &entry->node, hnode_equal);
        assert(hnode == &entry->node);

        fprintf(stderr, "key expired: %s\n", entry->key.c_str());

        entry_del(entry);
        entry_set_ttl(server.db, entry, -1); // remove from heap
    }
}

void entry_set_ttl(Database& db, Entry* entry, int64_t ttl_ms)
{
    if (ttl_ms < 0 && entry->heap_idx != (size_t)-1) {
        // negative TTL means remove it
        heap_delete(db.heap, entry->heap_idx);
        entry->heap_idx = -1;
    } else if (ttl_ms >= 0) {
        uint64_t expires_at = ttl_ms + Timer::get_monotonic_msec();
        heap_insert(db.heap, expires_at, &entry->heap_idx);
    }
}
