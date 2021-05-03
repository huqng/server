obj = main.o server.o threadpool.o http.o utils.o
cc = gcc

server: $(obj)
	$(cc) $(obj) -pthread -o server

main.o: server.h
server.o: server.h threadpool.h http.h utils.h
threadpool.o: threadpool.h utils.h
http.o: http.h utils.h
utils.o: utils.h

.PHONY:
clean:
	-rm server $(obj)