#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <pthread.h>

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

#define LOG_INFO 	0
#define LOG_ERR 	0
#define LOG_DEBUG	0

#define log_info(M, ...)	if(LOG_INFO)fprintf(stderr,		"[INFO]__[%0lX] (%s:%d) " M "\n", pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__)
#define log_err(M, ...)		if(LOG_ERR)fprintf(stderr,		"[ERR]___[%0lX] (%s:%d) " M "\n", pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__)
#define log_debug(M, ...)	if(LOG_DEBUG)fprintf(stderr,	"[DEBUG]_[%0lX] (%s:%d) " M "\n", pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__)

#endif