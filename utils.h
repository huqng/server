#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/fcntl.h>

/* File-reader with buffer */

#define FR_BUF_SIZE 1024

typedef struct file_reader{
	char buf[FR_BUF_SIZE];
	int index;
	int end;
	int fd;
}file_reader;

/* initialize a file-reader, return 0 if succeed or -1 if fail (file closed)*/
int fr_init(file_reader* fr, int fd);

/* read a byte from fr->fd, return 0 if succeed or -1 if fail */
int fr_read_byte(file_reader* fr, char* c);

/* return n bytes from fr->fd, = n * fr_read_byte */
int fr_read_n(file_reader* fr, char* c, int n);

#define log_info(M, ...)	fprintf(stderr, "[INFO][%0X] (%s:%d) " M "\n", pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__)
#define log_err(M, ...)		fprintf(stderr, "[ERR] [%0X] (%s:%d) " M "\n", pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__)

#endif