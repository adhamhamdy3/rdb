CPPFLAGS = -Wall -Wextra -O2 -g

run: clean a

a: server.o client.o
	@g++ $(CPPFLAGS) server.o -o a
	@g++ $(CPPFLAGS) client.o -o a
	@chmod +x a

server.o: server.cpp
	@g++ $(CPPFLAGS) -c server.cpp

client.o: client.cpp
	@g++ $(CPPFLAGS) -c client.cpp

clean:
	@rm -f *.o a