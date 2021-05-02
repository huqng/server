#ifndef _HTTP_H
#define _HTTP_H

#define RESOURCE_DIR "."

int http_request_head_parse_0(int fd, char** method, char** url, char** version);

void get_resource_path(char** path, char* url);

#endif