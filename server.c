#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <time.h>

#include "utils.h"
#include "threadpool.h"
#include "http.h"

#define MAX_TH_NUM 2
#define MAX_BUF_LEN 1024
#define SERVER_PORT 10000

typedef struct sockaddr_in sockaddr_in;

void* handle_request(void* arg);

void set_sin_server(sockaddr_in* sin, int port){
	memset(sin, 0, sizeof(sockaddr_in));
	sin->sin_addr.s_addr = htonl(INADDR_ANY);
	sin->sin_family = AF_INET;
	sin->sin_port = htons(port);
}

void set_sin_client(sockaddr_in* sin) {
	memset(sin, 0, sizeof(sockaddr_in));
	sin->sin_addr.s_addr = htonl(INADDR_ANY);
	sin->sin_family = AF_INET;
	sin->sin_port = htons(0);
}

int main(int argc, char **argv){
	/* create socket of server */
	int server_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (server_sock < 0) {
		perror("create socket");
		close(server_sock);
		exit(-1);
	}

	/* get sockaddr of server */
	struct sockaddr_in sin_server;
	set_sin_server(&sin_server, SERVER_PORT);
	int sin_server_len = sizeof(sin_server);

	/* bind server socket & sockaddr*/
	bind(server_sock, (const struct sockaddr*)&sin_server, sizeof(sin_server));

	/* listen */
	if (listen(server_sock, 10) < 0) {
		perror("listen");
		close(server_sock);
		exit(-1);
	}
	printf("Listening at port %d\n", SERVER_PORT);

	/* create a threadpool */
	threadpool* pool = threadpool_create(MAX_TH_NUM);

	/* listening */
	while (1){
		/* get sockaddr of client */
		struct sockaddr_in sin_client;
		set_sin_client(&sin_client);
		int sin_client_len = sizeof(sin_client);
		
		/* accept requests from client & get a new socket */
		int accept_sock = accept(server_sock, (struct sockaddr* restrict)&sin_client, (socklen_t* restrict)&sin_client_len);
		if (accept_sock < 0) {
			perror("accept");
			close(server_sock);
			exit(-1);
		}
		log_info("accepted a connect");
		int flags = fcntl(accept_sock, F_GETFL, 0);
		if(flags < 0){
			perror("fcntl-getfl");
			exit(-1);
		}
		if(fcntl(accept_sock, F_SETFL, flags | O_NONBLOCK) < 0){
			perror("fcntl-setfl");
			exit(-1);
		}

		/* make a task and assign it to threadpool */
		tp_task* task = threadpool_create_tp_task(handle_request, &accept_sock);
		threadpool_addtask(pool, task);
	}
	log_err("fatal error");
	threadpool_destroy(pool);
	return 0;
}

void* handle_request(void* arg){
	log_info("handling");
	int sock_fd = *(int*)arg;
	char* method;
	char* url;
	char* version;
	/* get method, url and version */
	if(http_request_head_parse_0(sock_fd, &method, &url, &version) < 0){
		log_info("fail to parse http request head");
		return NULL;
	}
	else
		log_info("parse: [%s] [%s] [%s]", method, url, version);

	char* filename;
	get_resource_path(&filename, url);

	if(!strcasecmp(version, "HTTP/1.1") || !strcasecmp(version, "HTTP/1.0")){
		if(!strcasecmp(method, "GET")) {
			FILE* fp = fopen(filename, "r");
			
			if(fp == NULL) {
				log_info("fail to open file [%s]", filename);
				// TODO - 404
			}
			else{
				/* succeed to open file */
				char head[] = 
					"HTTP/1.1 200 OK\r\n"
					"Server: aaaaaaa\r\n"
					"Content-Type: text/html\r\n\r\n";
				send(sock_fd, head, strlen(head), 0);

				if(send_file(fp, sock_fd) < 0) {
					log_err("fail to send file [%s]", filename);
				}
				else
					log_info("file sent [%s]", filename);

				fclose(fp);
			}
		}
		else if(!strcasecmp(method, "POST")) {
			log_err("Unimplemented HTTP version [%s]", method);
		}
		else {	
			log_err("Unimplemented HTTP version [%s]", method);
		}
	}
	else{
		log_err("Unimplemented HTTP version [%s]", version);
	}

	close(sock_fd);
	free(filename);
	free(method);
	free(url);
	free(version);
	return NULL;
}

