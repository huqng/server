OBJ = server.o threadpool.o http.o

server: $(OBJ)
	gcc $(OBJ) -pthread -o server

server.o: server.c
	gcc -c server.c 
threadpool: threadpool.c threadpool.h
	gcc -c threadpool.c
http.o: http.c http.h
	gcc -c http.c

client: client.c
	gcc client.c -o client -pthread

.PHONY:
clean:
	rm server client $(OBJ)