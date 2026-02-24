# rdb — Redis-like Database Engine

> **Work in Progress** — This project is under active development. The current `master` branch is functional and supports the core key-value operations described below. Additional features and refinements are ongoing.

---

## Overview

**rdb** is a lightweight, Redis-compatible in-memory key-value store written in C/C++. It implements a non-blocking, event-driven TCP server capable of handling multiple concurrent client connections using `poll(2)`, backed by a custom progressive hash map for O(1) average-case data operations.

The project is designed for learning and exploration of low-level systems programming — including network I/O, custom data structures, and binary protocol design.

---

## Features (Current `master`)

| Feature | Status |
|---|---|
| TCP server with non-blocking I/O via `poll` | Done |
| Multiple concurrent client connections | Done |
| Custom binary request/response protocol | Done |
| `GET` — retrieve a value by key | Done |
| `SET` — insert or update a key-value pair | Done |
| `DEL` — delete a key | Done |
| Custom progressive hash map (incremental rehashing) | Done |
| AVL tree implementation | In Progress |
| Docker support | Done |

---

## Architecture

```
rdb/
├── redis_server.cpp          # Server entry point
├── redis_client.cpp          # Client entry point
│
├── include/                  # Public headers
│   ├── net/
│   │   ├── server.h          # Server struct & event-loop interface
│   │   ├── client.h          # Client connection interface
│   │   └── protocol.h        # Type tags, error codes, serialization API
│   └── storage/
│       ├── rdb.h             # Database struct & command handlers
│       ├── hashtable.h       # HTable / HMap data structures
│       ├── avl.h             # AVL tree
│       └── buffer.h          # I/O ring buffer
│
├── src/                      # Implementations
│   ├── net/
│   │   ├── server.cpp        # Event loop, connection state machine
│   │   ├── client.cpp        # Client send/recv logic
│   │   └── protocol.cpp      # Request serialization / response deserialization
│   └── storage/
│       ├── rdb.cpp           # GET / SET / DEL handlers
│       ├── hashtable.cpp     # Progressive rehashing hash map
│       ├── avl.cpp           # AVL tree rotations & balancing
│       └── buffer.cpp        # Buffer read/write helpers
│
└── lib/                      # Internal utility libraries
    ├── io/
    │   ├── network_io.h      # Low-level socket read/write helpers
    │   ├── buffer_util.h     # Buffer utility functions
    │   └── parser.h          # Binary request parser
    └── log/
        └── logger.h          # Lightweight logger
```

### Key Design Decisions

- **Non-blocking I/O with `poll`** — The server uses a single-threaded event loop. Each client connection is tracked by a `connection_state` struct holding incoming/outgoing `Buffer`s and I/O flags (`want_read`, `want_write`, `want_close`). This avoids the complexity of threads while still serving multiple clients concurrently.

- **Progressive Rehashing** — The `HMap` structure maintains two internal `HTable`s (`older` and `newer`). When a resize threshold is hit, entries are migrated incrementally across requests (tracked via `migrate_pos`), avoiding stop-the-world rehashing latency spikes.

- **Custom Binary Protocol** — Requests and responses are length-prefixed binary frames (not plaintext). Type tags (`NIL`, `ERR`, `STR`, `INT`, `DBL`, `ARR`) allow the protocol to carry typed responses. Max server message size is 32 MiB; max client message is 4 KiB.

- **FNV Hash** — Keys are hashed using the FNV-1a algorithm for fast, well-distributed hashing.

---

## Getting Started

### Prerequisites

- `g++` with C++23 support (GCC 13+ recommended)
- GNU `make`

### Build

```bash
make
```

This produces two binaries: `redis_server` and `redis_client`.

### Run the Server

```bash
./redis_server
```

The server starts listening on **port 1234** by default.

### Use the Client

Send commands from a separate terminal:

```bash
./redis_client set foo bar
./redis_client get foo
./redis_client del foo
```

### Clean Build Artifacts

```bash
make clean
```

---

## Docker

A multi-stage `Dockerfile` is provided. It builds the server using GCC 13 and packages the resulting binary into a slim Debian image.

```bash
# Build the image
docker build -t rdb .

# Run the server (exposes port 1234)
docker run -p 1234:1234 rdb
```

---

## Project Status & Roadmap

This project is still a work in progress. Planned additions include:

- `KEYS` command — list all stored keys
- TTL / key expiry support
- Sorted sets backed by the AVL tree
- Persistence (RDB snapshots / AOF logging)
- Full RESP3 protocol compatibility
- Thread pool for parallel command execution

---

## License

This project is open source. License to be determined.
