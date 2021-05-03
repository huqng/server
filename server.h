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

#include "utils.h"
#include "threadpool.h"
#include "http.h"

#define MAX_TH_NUM 12
#define MAX_BUF_LEN 1024
#define SERVER_PORT 10000

typedef struct sockaddr_in sockaddr_in;

void* handle_request(void* arg);

void set_sin_server(sockaddr_in* sin, int port);

void set_sin_client(sockaddr_in* sin);

int run_server(int nth);

#endif
