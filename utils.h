#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/time.h>


/* File-reader with buffer */

#define FR_BUF_SIZE 256


typedef struct fd_reader{
	char buf[FR_BUF_SIZE];
	int index;
	int end;
	int fd;
}fd_reader;

int pass();

/* initialize a file-reader, return 0 if succeed or -1 if fail (file closed)*/
int fr_init(fd_reader* fr, int fd);

/* read a byte from fr->fd, return 0 if succeed or -1 if fail */
int fr_read_byte(fd_reader* fr, char* c);

/* return n bytes from fr->fd, = n * fr_read_byte */
int fr_read_n(fd_reader* fr, char* c, int n);

void make_fd_nonblocking(int fd);

/* these could be configured in main() */
extern int use_log_info;
extern int use_log_err;
extern int use_log_debug;

#define log_info LOG_INFO
#define log_err LOG_ERR
#define log_debug LOG_DEBUG

#define LOG_INFO(M, ...)	((int(*)(FILE*, const char*, ...))(use_log_info ? (void*)fprintf : (void*)pass)) \
	(stderr, "[INFO]__[%0lX] (%s:%d) " M "\n", pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_ERR(M, ...)		((int(*)(FILE*, const char*, ...))(use_log_err ? (void*)fprintf : (void*)pass)) \
	(stderr, "[ERR]___[%0lX] (%s:%d) " M "\n", pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_DEBUG(M, ...)	((int(*)(FILE*, const char*, ...))(use_log_debug ? (void*)fprintf : (void*)pass)) \
	(stderr, "[DEBUG]_[%0lX] (%s:%d) " M "\n", pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__)

#endif
