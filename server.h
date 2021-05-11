#ifndef _SERVER_H
#define _SERVER_H

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
#include <fcntl.h>

#include "threadpool.h"
#include "timer.h"
#include "http.h"

/* default configuration of server */
#define DEFAULT_PORT 10000
#define DEFAULT_NTH 24

#define LISTENQ 1024

typedef struct server_conf{
    uint16_t    port;
    int         nth;
}server_conf;

typedef struct sockaddr_in sockaddr_in;

/* configuration */
void server_conf_init(server_conf* conf);

void server_conf_set_port(server_conf* conf, int port);

void server_conf_set_epoll(server_conf* conf, int use_epoll);

/* call back function for threadpool */
void* server_handle_request(void* arg);

/* to run */
void server_set_sin_server(sockaddr_in* sin, int port);

void server_set_sin_client(sockaddr_in* sin);

int server_run(server_conf* conf);

/* do reply */
int http_handle_get_1_0(int sock_fd, const char* filename, http_request_t* req);

#endif
