#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/stat.h>

#include "threadpool.h"
#include "http.h"

#define MAX_BUF_LEN 1024
#define PORT 10000

void* accept_request(void* arg);
void handle_request(int sock, char* buf, int len);

struct sockaddr_in get_sin_server(int port){
	/* sockaddr of server socket */
	struct sockaddr_in sin_server;
	memset(&sin_server, 0, sizeof(sin_server));
	sin_server.sin_addr.s_addr = htonl(INADDR_ANY);
	sin_server.sin_family = AF_INET;
	sin_server.sin_port = htons(port);
	return sin_server;
}

int get_sock_server(){
	int sock_server = socket(PF_INET, SOCK_STREAM, 0);
	if (sock_server < 0)
	{
		perror("socket");
		close(sock_server);
		exit(-1);
	}
	return sock_server;
}

struct sockaddr_in get_sin_client(){
	/* sockaddr of client (any ip & any port) */
	struct sockaddr_in sin_client;
	memset(&sin_client, 0, sizeof(sin_client));
	sin_client.sin_addr.s_addr = htonl(INADDR_ANY);
	sin_client.sin_family = AF_INET;
	sin_client.sin_port = htons(0);
	return sin_client;
}

int main(int argc, char **argv){
	/* socket of server */
	int server_sock = get_sock_server();

	/* sockaddr of server */
	struct sockaddr_in sin_server = get_sin_server(PORT);
	int sin_server_len = sizeof(sin_server);

	/* bind server socket & sockaddr*/
	bind(server_sock, &sin_server, sizeof(sin_server));

	/* listen */
	if (listen(server_sock, 10) < 0) {
		perror("listen");
		close(server_sock);
		exit(-1);
	}
	printf("Listening...\n");

	threadpool* pool = threadpool_create();

	/* connection cnt */
	int n_conn = 0;
	/* loop for listening */
	while (1){
		/* sockaddr of client */
		struct sockaddr_in sin_client = get_sin_client();
		int sin_client_len = sizeof(sin_client);

		/* accept requests from client & get a new socket */
		int accept_sock = accept(server_sock, &sin_client, &sin_client_len);
		if (accept_sock < 0)
		{
			perror("accept");
			close(server_sock);
			exit(-1);
		}
		n_conn += 1;
		//printf("%d client(s) connected\n", n_conn);

		/* assign handle request to a thread*/
		tp_task* task = threadpool_create_tp_task(accept_request, &accept_sock);
		threadpool_addtask(pool, task);
	}
	return 0;
}

void* accept_request(void* arg){
	int accept_sock = *(int*)arg;
	char buf[MAX_BUF_LEN + 1];
	// loop for recv
	while(1){
		int recv_len = recv(accept_sock, buf, MAX_BUF_LEN, 0);
		if(recv_len < 0){
			perror("recv");
			close(accept_sock);
			exit(-1);
		}
		/* handle request */
		handle_request(accept_sock, buf, recv_len);

		//if(send(accept_sock, buf, recv_len, 0) < 0){
		//	perror("send");
		//	close(accept_sock);
		//	exit(-1);
		//}
	}
}

void handle_request(int sock, char* buf, int len){
	char* method;
	char* url;
	char* version;
	if(http_request_parse(buf, len, &method, &url, &version) < 0){
		perror("http request parse\n");
		exit(-1);
	}
	int v10 		= !strcasecmp(version, "HTTP/1.0");
	int v11 		= !strcasecmp(version, "HTTP/1.1");
	int mget 		= !strcasecmp(method, "GET");
	int mhead 		= !strcasecmp(method, "HEAD");
	int mpost 		= !strcasecmp(method, "POST");
	int mput 		= !strcasecmp(method, "PUT");
	int mdelete 	= !strcasecmp(method, "DELETE");
	int mconnect 	= !strcasecmp(method, "CONNECT");
	int moptions 	= !strcasecmp(method, "OPTIONS");
	int mtrace 		= !strcasecmp(method, "TRACE");
	int mpatch 		= !strcasecmp(method, "PATCH");
	if(v10 || v11){
		if(mget){
			char path[512];
			int slen = sprintf(path, ".%s", url);
			if(path[slen - 1] == '/')
				strcat(path, "index.html");
			FILE* resource = fopen(path, "r");
			if(resource == NULL){
				perror("error fopen");
				exit(-1);
			}
			else{
				strcpy(buf, "HTTP/1.0 200 OK\r\n");
				send(sock, buf, strlen(buf), 0);
				strcpy(buf, "SERVER_STRING");
				send(sock, buf, strlen(buf), 0);
				sprintf(buf, "Content-Type: text/html\r\n");
				send(sock, buf, strlen(buf), 0);
				strcpy(buf, "\r\n");
				send(sock, buf, strlen(buf), 0);
				fgets(buf, sizeof(buf), resource);
				while(!feof(resource)){
					int sent = send(sock, buf, strlen(buf), 0);
					fgets(buf, sizeof(buf), resource);
				}
			}
		}
		else{
			perror("error http method");
			free(method);
			free(url);
			free(version);
			exit(-1);
		}
	}
	else{
		perror("error http version");
		free(method);
		free(url);
		free(version);
		exit(-1);
	}
}
