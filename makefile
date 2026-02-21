CXX      := g++
CXXFLAGS := -std=c++23 -g -Iinclude -Ilib

COMMON_SRC := src/net/client.cpp src/net/server.cpp \
              src/storage/hashtable.cpp src/storage/rdb.cpp \
			  src/net/protocol.cpp src/storage/buffer.cpp

all: redis_server redis_client

redis_server: redis_server.cpp $(COMMON_SRC)
	@$(CXX) $(CXXFLAGS) -o $@ $^

redis_client: redis_client.cpp $(COMMON_SRC)
	@$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	@rm -f redis_server redis_client