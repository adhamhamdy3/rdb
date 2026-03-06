# rdb

A Redis-compatible in-memory key-value database server written in C/C++, built from scratch. It implements custom core data structures, a binary serialization protocol, and an asynchronous single-threaded event loop — without relying on the STL for its internals.

## Features

- **Custom data structures** — intrusive doubly linked lists, hash maps, AVL trees, and a min-heap, all implemented from scratch
- **Binary protocol** — custom tagged serialization (`TAG_STR`, `TAG_INT`, `TAG_ARR`, etc.) over TCP rather than RESP
- **Async event loop** — single-threaded, `poll()`-based multiplexing with non-blocking I/O and read/write state machines
- **Supported types** — string key-value pairs and sorted sets (ZSet)
- **TTL / expiration** — per-key millisecond TTL backed by a min-heap; expired keys are evicted on each event loop tick
- **Idle connection timeouts** — connections inactive for more than 5 seconds are automatically closed via an intrusive idle queue

## Project Structure

```
.
├── redis_server.cpp        # Server entry point
├── redis_client.cpp        # Client entry point
├── src/
│   ├── net/                # server, client, protocol, timers
│   └── storage/
│       ├── core/           # hashtable, avl, heap, list
│       ├── rdb.cpp         # core database logic and command dispatch
│       ├── zset.cpp        # sorted set operations
│       ├── buffer.cpp      # I/O buffer management
│       └── entry.cpp       # key-value entry representation
└── include/                # header files mirroring src/ layout
```

## Supported Commands

### Key-Value (String)

| Command | Description |
|---|---|
| `set <key> <value>` | Store a string value |
| `get <key>` | Retrieve a string value |
| `del <key>` | Delete a key (any type); returns `1` if removed, `0` if not found |
| `keys` | Return all keys (O(N)) |

### Sorted Set (ZSet)

| Command | Description |
|---|---|
| `zadd <key> <score> <name>` | Add or update a member with a float score |
| `zrem <key> <name>` | Remove a member |
| `zscore <key> <name>` | Get the score of a member |
| `zquery <key> <score> <name> <offset> <limit>` | Range query from a lower bound; returns alternating `name, score` pairs |

### Expiration

| Command | Description |
|---|---|
| `pexpire <key> <ttl_ms>` | Set a TTL in milliseconds |
| `pttl <key>` | Get remaining TTL in ms; `-1` if no expiry, `-2` if key missing |

## Building Locally

**Requirements:** GCC/G++, `make`

```bash
make          # builds both ./rdb (server) and ./client
make clean    # removes build artifacts
```

Run the server:

```bash
./rdb --host 0.0.0.0 --port 1234
```

Run the client:

```bash
./client --host 127.0.0.1 --port 1234
```

## Running with Docker

**Requirements:** Docker, Docker Compose

### Start the server

```bash
docker compose up server
```

The server will be available at `localhost:1234`.

### Run the client

```bash
docker compose run --rm client
```

Starts an interactive client session connected to the running server.

### Build the image manually

```bash
docker build -t rdb .
docker run -p 1234:1234 rdb
```

## Configuration

| Flag | Default | Description |
|---|---|---|
| `--host` | `127.0.0.1` | Address to bind (server) or connect to (client) |
| `--port` | `1234` | Port to bind or connect on |

## Architecture Notes

**Event loop** — `event_loop()` in `server.h` uses `poll()` to multiplex all connections on a single thread. Connection state is managed via `conn_state_map`, enabling O(1) lookups. Each connection progresses through `want_read` / `want_write` states handled by `handle_accept`, `handle_read`, and `handle_write`.

**Timers** — Two timer mechanisms run on each loop tick via `process_timers()`:
- *Idle timeouts*: an intrusive `DList idle_queue` tracks last-active time per connection; connections at the head exceeding `MAX_IDLE_TIMEOUT` (5s) are closed.
- *Key expiration*: a min-heap stores absolute expiry timestamps in milliseconds; keys at the top are deleted when their deadline has passed.

## License

MIT
