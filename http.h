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
#define RESPONSE_404_HTML "./html/404.html"

typedef enum http_parse_state_t{
	hp_s_method = 0,
	hp_s_space_1,
	hp_s_url,
	hp_s_space_2,
	hp_s_version,
	hp_s_error,
	hp_s_line_cr,
	hp_s_line_nl,
	hp_s_key,
	hp_s_key_end,
	hp_s_val,
	hp_s_val_end,
	hp_s_finish,
	hp_s_next_key
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
	int connection;
}http_request_t;

int http_request_init(http_request_t* r);

int http_request_parse(int fd, http_request_t* req);

int http_parse_get_value(fd_reader* fr, char* buf, int bufsize);

int http_request_set_kv(char* key, char* value, http_request_t* req);

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


void get_resource_path(char** path, char* url);

int send_file(FILE* fp, int sock_fd);

http_request_method_t get_http_request_method(const char* method);

http_version_t get_http_version(const char* version);

char* get_content_type(char* filename);

#endif
