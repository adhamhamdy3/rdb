CPPFLAGS = -Wall -Wextra -O2 -g

all: server client

server: server.o
	@g++ $(CPPFLAGS) server.o -o server

client: client.o
	@g++ $(CPPFLAGS) client.o -o client

server.o: server.cpp
	@g++ $(CPPFLAGS) -c server.cpp

client.o: client.cpp
	@g++ $(CPPFLAGS) -c client.cpp

clean:
	@rm -f server client server.o client.o