#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define MAX_LEN 1024

void* accept_request(void* arg);
void modify_buf(char* buf, int len);


int main(int argc, char **argv)
{
	int server_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (server_sock < 0)
	{
		perror("socket");
		close(server_sock);
		exit(-1);
	}

	// sockaddr of server socket
	struct sockaddr_in sin_server;
	memset(&sin_server, 0, sizeof(sin_server));
	sin_server.sin_addr.s_addr = htonl(INADDR_ANY);
	sin_server.sin_family = AF_INET;
	sin_server.sin_port = htons(10000);
	int sin_server_len = sizeof(sin_server);
	// bind server socket
	bind(server_sock, &sin_server, sizeof(sin_server));
	// listen
	if (listen(server_sock, 10) < 0)
	{
		perror("listen");
		close(server_sock);
		exit(-1);
	}
	printf("Listening...\n");

	int n_conn = 0;
	// loop for listening
	while (1)
	{
		// sockaddr of client (any ip and any port)
		struct sockaddr_in sin_client;
		memset(&sin_client, 0, sizeof(sin_client));
		sin_client.sin_addr.s_addr = htonl(INADDR_ANY);
		sin_client.sin_family = AF_INET;
		sin_client.sin_port = htons(0);
		int sin_client_len = sizeof(sin_client);
		// accept request from client & get a new socket
		int accept_sock = accept(server_sock, &sin_client, &sin_client_len);
		if (accept_sock < 0)
		{
			perror("accept");
			close(server_sock);
			exit(-1);
		}
		n_conn += 1;
		printf("%d client(s) connected\n", n_conn);
		// create a new thread to handle request
		pthread_t new_thread;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

		pthread_create(&new_thread, &attr, accept_request, &accept_sock);
	//	pthread_join(new_thread, NULL);

	}
	return 0;
}


void* accept_request(void* arg){
	int accept_sock = *(int*)arg;
	char buf[MAX_LEN + 1];
	// loop for recv
	while(1){
		int recv_len = recv(accept_sock, buf, MAX_LEN, 0);
		if(recv_len < 0){
			perror("recv");
			close(accept_sock);
			exit(-1);
		}
		/* modify received msg in buf */
		modify_buf(buf, recv_len);

		if(send(accept_sock, buf, recv_len, 0) < 0){
			perror("send");
			close(accept_sock);
			exit(-1);
		}
	}
}

void modify_buf(char* buf, int len){
	for(int i = 0; i < len; i++){
		buf[i] = buf[i] % 26 + 'A';
	}
}