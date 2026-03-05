#include "net/server.h"

void socket_set_nb(int socket)
{
    int flags = fcntl(socket, F_GETFL, 0);

    // O_NONBLOCK flag applies to: read() - write() - accept() - connect()
    flags |= O_NONBLOCK;
    fcntl(socket, F_SETFL, flags);
}

// TODO: migrate timers

int32_t next_timer_ms(Server& server)
{
    // TODO: separate
    uint64_t now_ms = Timer::get_monotonic_msec();
    uint64_t next_ms = (uint64_t)-1; // infinity

    // check idle connections
    if (!dlist_isempty(&server.idle_queue)) {
        connection_state* conn = container_of(server.idle_queue.next, connection_state, idle_node);
        next_ms = conn->last_active_ms + MAX_IDLE_TIMEOUT;
    }

    // check TTLs timers
    if (!server.heap.heap_arr.empty() && server.heap.heap_arr[0].value < next_ms) {
        next_ms = server.heap.heap_arr[0].value;
    }

    if (next_ms == (uint64_t)-1) {
        return -1; // no timers, wait
    }

    if (next_ms <= now_ms) {
        return 0;
    }

    return int32_t(next_ms - now_ms);
}

bool hnode_equal(HNode* l, HNode* r)
{
    return l == r;
}

void reset_timers(Server& server, connection_state* conn)
{
    dlist_detach(&conn->idle_node);
    conn->last_active_ms = Timer::get_monotonic_msec();
    dlist_insert_before(&server.idle_queue, &conn->idle_node);
}

void process_timers(Server& server)
{
    // TODO: separate

    // idle timers
    uint64_t now_ms = Timer::get_monotonic_msec();
    while (!dlist_isempty(&server.idle_queue)) {
        connection_state* conn = container_of(server.idle_queue.next, connection_state, idle_node);
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

    auto const& heap_arr = server.heap.heap_arr;
    while (!heap_arr.empty() && heap_arr[0].value < now_ms && works++ < max_work) {
        Entry* entry = container_of(heap_arr[0].ref, Entry, heap_idx);

        HNode* hnode = hm_delete(&server.db.hashmap, &entry->node, hnode_equal);
        assert(hnode == &entry->node);

        fprintf(stderr, "key expired: %s\n", entry->key.c_str());

        entry_del(entry);
    }
}

void entry_set_ttl(Server& server, Entry* entry, int64_t ttl_ms)
{
    if (ttl_ms < 0 && entry->heap_idx != (size_t)-1) {
        // negtive TTL means remove it
        heap_delete(server.heap, entry->heap_idx);
        entry->heap_idx = -1;
    } else if (ttl_ms >= 0) {
        uint64_t expires_at = ttl_ms + Timer::get_monotonic_msec();
        heap_insert(server.heap, expires_at, &entry->heap_idx);
    }
}

connection_state* conn_new(Server& server, int connection_socket)
{
    // create connection state for this socket
    connection_state* conn = new connection_state();
    conn->tcp_socket = connection_socket;
    conn->want_read = true; // read the first request

    // timers
    conn->last_active_ms = Timer::get_monotonic_msec();
    dlist_insert_before(&server.idle_queue, &conn->idle_node);
}

void conn_destroy(Server& server, connection_state* conn)
{
    close(conn->tcp_socket);
    server.conn_state_map[conn->tcp_socket] = nullptr;
    dlist_detach(&conn->idle_node);

    delete conn;
}

void process_command(std::vector<std::string> const& command, Buffer& buf, Database& db)
{
    if (command.size() == 2 && command[0] == "get") {
        do_get(command, buf, db);
    } else if (command.size() == 2 && command[0] == "del") {
        do_del(command, buf, db);
    } else if (command.size() == 3 && command[0] == "set") {
        do_set(command, buf, db);
    } else if (command.size() == 1 && command[0] == "keys") {
        do_keys(command, buf, db);
    } else if (command.size() == 4 && command[0] == "zadd") {
        do_zadd(command, buf, db);
    } else if (command.size() == 3 && command[0] == "zrem") {
        do_zrem(command, buf, db);
    } else if (command.size() == 3 && command[0] == "zscore") {
        do_zscore(command, buf, db);
    } else if (command.size() == 6 && command[0] == "zquery") {
        do_zquery(command, buf, db);
    } else {
        buf_out_err(buf, TAG_ERR, ERR_UNKNOWN, "unknown command.");
    }
}

bool try_one_request(connection_state* conn, Database& db)
{
    // message size
    if (conn->incoming.data.size() < 4) {
        return false;
    }

    uint32_t len = 0;
    memcpy(&len, conn->incoming.data.data(), 4);
    if (len > SMAX_MSG_LENGTH) {
        Logger::alert("message too long");
        conn->want_close = true;
        return false;
    }

    // message body
    if (4 + len > conn->incoming.data.size()) {
        return false; // request body is not fully received yet
    }

    uint8_t* request = &conn->incoming.data[4];

    std::vector<std::string> command;
    if (Protocol::deserialize_request(request, len, command) < 0) {
        Logger::alert("bad request");
        conn->want_close = true;
        return false;
    }

    size_t header = 0;
    Protocol::response_begin(conn->outgoing, &header);

    process_command(command, conn->outgoing, db);

    Protocol::response_end(conn->outgoing, header);

    // remove the message from the incoming buffer
    buffer_consume(conn->incoming, 4 + len);

    return true;
}

connection_state* handle_accept(Server& server, int tcp_socket)
{
    struct sockaddr_in client_socket = {};
    socklen_t client_address = sizeof(client_socket);

    int connection_socket = accept(tcp_socket, (struct sockaddr*)&client_socket, &client_address);
    if (connection_socket < 1) {
        return nullptr;
    }

    uint32_t ip = client_socket.sin_addr.s_addr;
    fprintf(stderr, "new client from %u.%u.%u.%u:%u\n",
        ip & 255, (ip >> 8) & 255, (ip >> 16) & 255, ip >> 24,
        ntohs(client_socket.sin_port));

    // set this connection socket to non-blocking
    socket_set_nb(connection_socket);

    connection_state* conn = conn_new(server, connection_socket);

    if (conn) {
        if (server.conn_state_map.size() <= (size_t)conn->tcp_socket) {
            server.conn_state_map.resize(conn->tcp_socket + 1, nullptr);
        }

        assert(!server.conn_state_map[conn->tcp_socket]);
        server.conn_state_map[conn->tcp_socket] = conn;
    }

    return conn;
}

// app logic when socket is readable
void handle_read(connection_state* conn, Database& db)
{
    uint8_t buffer[64 * 1024];
    // TODO: set timeout for read
    ssize_t ret_val = read(conn->tcp_socket, buffer, sizeof(buffer));
    if (ret_val < 0 && errno == EAGAIN) {
        return; // not ready
    }

    if (ret_val < 0) {
        Logger::alert("read() error");
        conn->want_close = true;
        return;
    }

    // handle EOF
    if (ret_val == 0) {
        if (conn->incoming.data.size() == 0) {
            Logger::alert("client closed");
        } else {
            Logger::alert("unexpected EOF");
        }
        conn->want_close = true;
        return;
    }

    // accumulate the new data to connection state incoming data buffer
    buffer_append(conn->incoming, buffer, ret_val);

    // try to parse the accumulated data
    while (try_one_request(conn, db)) {
        /* do nothing */
    }

    // state transition
    if (conn->outgoing.data.size() > 0) {
        conn->want_write = true;
        conn->want_read = false;

        // Instead of waiting for the next loop iteration (extra syscall) and a POLLOUT
        // try to write now
        return handle_write(conn); // optimization
    }
}

// app logic when socket is writable
void handle_write(connection_state* conn)
{
    assert(conn->outgoing.data.size() > 0);

    // TODO: set timeout for write
    int ret_val = write(conn->tcp_socket, conn->outgoing.data.data(), conn->outgoing.data.size());
    if (ret_val < 0 && errno == EAGAIN) {
        return; // not ready
    }

    if (ret_val < 0) {
        Logger::alert("write() error");
        conn->want_close = true;
        return;
    }

    // remove written data from the outgoing buffer
    buffer_consume(conn->outgoing, ret_val);

    // state transition
    if (conn->outgoing.data.size() == 0) {
        conn->want_read = true;
        conn->want_write = false;
    }
}

void init_server_socket(Server& server)
{
    // create tcp socket
    server.socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server.socket < 0) {
        Logger::log_error("socket()");
    }

    int reuse_option = 1;
    setsockopt(server.socket, SOL_SOCKET, SO_REUSEADDR, &reuse_option, sizeof(reuse_option));
}

void init_server_address(Server& server, uint16_t port_number, uint32_t ip_address)
{
    // server address (IP + port)
    // htons & htonl swaps from big-endian to little-endian
    server.address.sin_family = AF_INET; // IPv4
    server.address.sin_port = htons(port_number);
    server.address.sin_addr.s_addr = htonl(ip_address);

    int ret_val = bind(server.socket, (const struct sockaddr*)&server.address, sizeof(server.address));
    if (ret_val) {
        Logger::log_error("bind()");
    }

    // set server socket to non-blocking mode
    socket_set_nb(server.socket);
}

void server_start_listen(Server& server)
{
    // listen
    int ret_val = listen(server.socket, SOMAXCONN); // SOMAXCONN: size of the queue
    if (ret_val) {
        Logger::log_error("listen()");
    }

    Logger::alert("server is listening...");

    dlist_init(&server.idle_queue);
    event_loop(server);
}

void event_loop(Server& server)
{
    // event loop
    // accept connections
    /// rebuild the sockets list every iteration based on the application logic's intent read/write/error
    while (true) {
        server.sockets_list.clear();

        struct pollfd socket = { server.socket, POLLIN, 0 }; // index 0: listening socket, non-blocking accept()
        server.sockets_list.push_back(socket);

        for (connection_state* conn : server.conn_state_map) {
            if (!conn) {
                continue;
            }

            struct pollfd sock = { conn->tcp_socket, POLLERR, 0 }; // always wake up if error happened

            if (conn->want_read) {
                sock.events |= POLLIN;
            }
            if (conn->want_write) {
                sock.events |= POLLOUT;
            }

            server.sockets_list.push_back(sock);
        }

        /// blocking - check all sockets receive/send buffers or TCP errors, -1 wait forever
        /// timeout option: "wake me up in at most X milliseconds, even if no IO is ready."
        int32_t timeout_ms = next_timer_ms(server);
        int ret_val = poll(server.sockets_list.data(), (nfds_t)server.sockets_list.size(), timeout_ms);
        if (ret_val < 0 && errno == EINTR) { // nothing is ready rn
            continue;
        }
        if (ret_val < 0) {
            Logger::log_error("poll()");
        }

        if (server.sockets_list[0].revents & POLLIN) {
            int listening_socket = server.sockets_list[0].fd;

            connection_state* conn = handle_accept(server, listening_socket);
        }

        // handle connection socket
        for (size_t i = 1; i < server.sockets_list.size(); i++) {
            connection_state* conn = server.conn_state_map[server.sockets_list[i].fd];

            uint32_t revents = server.sockets_list[i].revents;

            if (revents == 0) {
                continue; // no activity, don't reset timers
            }

            reset_timers(server, conn); //  only reset on actual IO

            if (revents & POLLIN) {
                assert(conn->want_read);
                handle_read(conn, server.db);
            }

            if (revents & POLLOUT) {
                assert(conn->want_write);
                handle_write(conn);
            }

            if ((revents & POLLERR) || conn->want_close) {
                conn_destroy(server, conn);
            }
        }

        process_timers(server);
    }
}
