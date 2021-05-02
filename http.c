 #include "http.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>


/* utility to read file */
#define FR_BUF_SIZE 512

typedef struct file_reader{
	char buf[FR_BUF_SIZE];
	int index;
	int end;
	int fd;
}file_reader;

int fr_init(file_reader* fr, int fd){
	fr->index = 0;
	fr->end = 1;
	fr->fd = fd;

	int n = read(fr->fd, fr->buf, FR_BUF_SIZE);
	if(n < 0){
		/* error */
		perror("read from sockfd");
		exit(-1);
	}
	else if(n == 0){
		/* closed */
		return -1;
	}
	else{
		/* success */
		fr->end = n;
		fr->index = 0;
		return 0;
	}
}

int fr_read_byte(file_reader* fr, char* c){
	if(fr->index >= fr->end){
		if(fr->end < FR_BUF_SIZE) {
			/* eof */
			*c = 0;
			return -1;
		}
		else{
			int n = read(fr->fd, fr->buf, FR_BUF_SIZE);
			if(n < 0){
				perror("read from sockfd");
				exit(-1);
			}
			else if(n == 0){
				/* eof */
				*c = 0;
				return -1;
			}
			else{
				/* re-read & success */
				fr->index = 1;
				fr->end = n;
				*c = fr->buf[0];
				return 0;
			}
		}
	}
	else{
		/* success */
		*c = fr->buf[fr->index];
		fr->index++;
		return 0;
	}
}
/* ^^^^^  */

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

int http_request_head_parse_0(int fd, char** method, char** url, char** version){
    file_reader* fr = (file_reader*)malloc(sizeof(file_reader));
	if(fr_init(fr, fd) < 0) {
		/* closed */
		return -2;
	}

	/* an FSM to parse http request [method][url][version] */
	int len_method, len_url, len_version;
	char c;
	int i = 0;
	char* buffer = (char*)malloc(4096);
	int state = hp_method;
    /* if state changed after reading, then don't need read again */
    int need_read = 1;
	while(1){
        if(need_read && fr_read_byte(fr, &c) < 0) {
            /* eof before finishing parsing */
            state = hp_error;
        }

		switch (state) {
		case hp_method:
			if(c == '\r' || c == '\n') {
                need_read = 0;
				state = hp_error;
			}
			else if(isspace(c)) {
                need_read = 0;
				state = hp_space_1;
				len_method = i;
				buffer[i] = 0;
				*method = (char*)malloc(i + 1);
				memcpy(*method, buffer, i + 1);
			}
			else {
                need_read = 1;
				buffer[i] = c;
				i++;
			}
            //printf("cur state hp_method i:%d\n", i);
			break;
		case hp_space_1:
			if(c == '\r' || c == '\n') {
				free(*method);
                need_read = 0;
				state = hp_error;
			}
			else if(!isspace(c)) {
                need_read = 0;
				state = hp_url;
				i = 0;
			}
            else{
                need_read = 1;
            }
            //printf("cur state hp_space_1 i:%d\n", i);
			break;
		case hp_url:
			if(c == '\r' || c == '\n') {
				free(*method);
                need_read = 0;
				state = hp_error;
			}
			else if(isspace(c)) {
                need_read = 0;
				state = hp_space_2;
				len_url = i;
				buffer[i] = 0;
				*url = (char*)malloc(i + 1);
				memcpy(*url, buffer, i + 1);
			}
			else {
                need_read = 1;
				buffer[i] = c;
				i++;
			}
            //printf("cur state hp_url i:%d\n", i);
			break;
		case hp_space_2:
			if(c == '\r' || c == '\n') {
				free(*method);
				free(*url);
                need_read = 0;
				state = hp_error;
			}
			else if(!isspace(c)) {
                need_read = 0;
				state = hp_version;
				i = 0;
			}
            else{
                need_read = 1;
            }
            //printf("cur state hp_space_2 i:%d\n", i);
			break;
		case hp_version:
			if(c == '\r' || c == '\n') {
				len_version = i;
				buffer[i] = 0;
				*version = (char*)malloc(i + 1);
				memcpy(*version, buffer, i + 1);
                need_read = 0;
				state = (c == '\r' ? hp_success_0 : hp_success);
				i = 0;
			}
			else if(isspace(c)) {
				free(*method);
				free(*url);
                need_read = 0;
				state = hp_error;
			}
			else {
                need_read = 1;
				buffer[i] = c;
				i++;
			}
            //printf("cur state hp_version i:%d\n", i);
			break;
		case hp_error:
            //printf("cur state hp_error i:%d\n", i);
			return -1;
		case hp_success_0:
			if(c == '\r') {
                need_read = 1;
				state = hp_success_0;
            }
            else if(c == '\n') {
                need_read = 0;
                state = hp_success;
            }
			else {
				free(*method);
				free(*url);
				free(*version);
                need_read = 0;
				state = hp_error;
			}
            //printf("cur state hp_success_0 i:%d\n", i);
			break;
		case hp_success:
            //printf("cur state hp_success i:%d\n", i);
			return 0;
		default:
			/* unexpected */
			break;
		}
	}
	perror("error parse");
	exit(-1);
}

void get_resource_path(char** path, char* url){

}