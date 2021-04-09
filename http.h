#ifndef _HTTP_H
#define _HTTP_H

int http_request_parse_1(char* buf, int len, char** method, char** url, char** version);

#endif