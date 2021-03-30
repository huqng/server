#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>

#define MAX_BUF_LEN 1024

int main(int argc, char** argv){
    int client_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(client_sock < 0){
        perror("err socket\n");
        exit(-1);
    }
    // sockaddr of client socket
    struct sockaddr_in sin_client;
    memset(&sin_client, 0, sizeof(sin_client));
    sin_client.sin_addr.s_addr = htonl(INADDR_ANY);
    sin_client.sin_family = AF_INET;
    sin_client.sin_port = htons(0);
    // bind client socket
    bind(client_sock, &sin_client, sizeof(sin_client));
    // sockaddr of server socket
    struct sockaddr_in sin_server;
    memset(&sin_server, 0, sizeof(sin_server));
    sin_server.sin_addr.s_addr = inet_addr("127.0.0.1");
    sin_server.sin_family = AF_INET;
    sin_server.sin_port = htons(10000);
    int sin_server_len = sizeof(sin_server);
    // connect to server
    if(connect(client_sock, &sin_server, sin_server_len) < 0){
        perror("connect");
        exit(-1);
    }


    char buf[MAX_BUF_LEN + 1];
    
    while(1){
        scanf("%s", buf);
        if(send(client_sock, buf, strlen(buf), 0) < 0){
            printf("ERROR1\n");
            perror("send");
            close(client_sock);
            exit(-1);
        }
        if(recv(client_sock, buf, MAX_BUF_LEN, 0) < 0){
            printf("ERROR2\n");
            perror("recv");
            close(client_sock);
            exit(-1);
        }
        printf(buf);
        printf("\n");
    }
    close(client_sock);
    return 0;


}