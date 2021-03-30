OBJ = server.o threadpool.o

server: $(OBJ)
	gcc $(OBJ) -pthread -o server

server.o: server.c
	#gcc -c server.c 
threadpool: threadpool.c threadpool.h
	#gcc -c threadpool

client: client.c
	gcc client.c -o client -pthread

.PHONY:
clean:
	rm server client $(OBJ)