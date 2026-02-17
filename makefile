CXX      := g++
CXXFLAGS := -std=c++23 -g -Iinclude -Iutil

COMMON_SRC := src/client.cpp src/hashtable.cpp src/rdb.cpp src/server.cpp

all: redis_server redis_client

redis_server: redis_server.cpp $(COMMON_SRC)
	@$(CXX) $(CXXFLAGS) -o $@ $^

redis_client: redis_client.cpp $(COMMON_SRC)
	@$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	@rm -f redis_server redis_client