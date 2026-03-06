CXX      := g++
CXXFLAGS := -std=c++23 -g -Iinclude -Ilib

COMMON_SRC := src/net/client.cpp src/net/server.cpp src/net/timers.cpp \
              src/storage/core/hashtable.cpp src/storage/rdb.cpp \
			  src/net/protocol.cpp src/storage/buffer.cpp \
			  src/storage/core/avl.cpp src/storage/zset.cpp \
			  src/storage/core/list.cpp src/storage/core/heap.cpp src/storage/entry.cpp

all: rdb client

rdb: redis_server.cpp $(COMMON_SRC)
	@$(CXX) $(CXXFLAGS) -o $@ $^

client: redis_client.cpp $(COMMON_SRC)
	@$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	@rm -f rdb client