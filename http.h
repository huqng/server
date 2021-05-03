#ifndef _HTTP_H
#define _HTTP_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <pthread.h>
#include "utils.h"

#define RESOURCE_ROOT_DIR "."

enum http_parse_state{
	hp_method = 0,
	hp_space_1,
	hp_url,
	hp_space_2,
	hp_version,
	hp_error,
	hp_success_0,
	hp_success
};

int http_request_head_parse_0(int fd, char** method, char** url, char** version);

void get_resource_path(char** path, char* url);

int send_file(FILE* fp, int sock_fd);

#endif