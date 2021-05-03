obj = server.o threadpool.o http.o utils.o
cc = gcc

server: $(obj)
	$(cc) $(obj) -pthread -o server

server.o: threadpool.h http.h
threadpool.o: threadpool.h
http.o: http.h
utils.o: utils.h

.PHONY:
clean:
	-rm server $(obj)