server: server.o threadpool.o
	gcc server.o threadpool.o -pthread -o server

client: client.c
	gcc client.c -o client -pthread

.PHONY:
clean:
	rm server client test *.o