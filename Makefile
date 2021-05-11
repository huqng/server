obj = main.o server.o threadpool.o http.o utils.o timer.o
cc = gcc

server: $(obj)
	$(cc) $(obj) -pthread -o server

main.o: server.h threadpool.h http.h utils.h timer.h
server.o: server.h threadpool.h http.h utils.h timer.h
threadpool.o: threadpool.h utils.h
http.o: http.h utils.h timer.h
utils.o: utils.h
timer.o: timer.h

.PHONY:
clean:
	-rm server $(obj)