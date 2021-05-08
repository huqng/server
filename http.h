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
#define RESPONSE_404_HTML "./404.html"

typedef enum http_parse_state_t{
	hp_method = 0,
	hp_space_1,
	hp_url,
	hp_space_2,
	hp_version,
	hp_error,
	hp_success_0,
	hp_success
}http_parse_state_t;

typedef enum http_request_method_t {
	http_m_get = 0,
	http_m_post,
	http_m_head,
	http_m_put,
	http_m_delete,
	http_m_connect,
	http_m_options,
	http_m_trace,
	http_m_patch,
	http_m_err
}http_request_method_t;

typedef enum http_version_t {
	http_v_1_0 = 0,
	http_v_1_1,
	http_v_err
} http_version_t;

typedef struct http_request_t {
	enum http_request_method_t method;
	char* url;
	http_version_t version;
}http_request_t;

/* http Response Head consts */
/* nl means New Line */
static char rh_status_200_nl[] 		= "HTTP/1.1 200 OK\r\n";
static char rh_status_404_nl[] 		= "HTTP/1.1 200 Not Found\r\n";
static char rh_server_nl[] 			= "Server: Nonserver\r\n";

static char rh_content_type[] 		= "Content-Type: ";
static char rh_content_type_html[] 	= "text/html";
static char rh_content_type_png[] 	= "img/x-png";
static char rh_content_type_icon[] 	= "img/x-icon";
static char rh_nl[] = "\r\n";

int http_request_head_parse_0(int fd, http_request_t* req);

void get_resource_path(char** path, char* url);

int send_file(FILE* fp, int sock_fd);

http_request_method_t get_http_request_method(const char* method);

http_version_t get_http_version(const char* version);

char* get_contene_type(char* filename);

#endif
