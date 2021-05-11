#include "utils.h"

int pass() {}

int fr_init(fd_reader* fr, int fd){
	fr->index = 0;
	fr->end = 1;
	fr->fd = fd;

	int n = recv(fd, fr->buf, FR_BUF_SIZE, 0);

	if(n < 0){
		/* closed */
		return -2;
	}
	else if(n == 0){
		/* eof */
		return -1;
	}
	else{
		/* success */
		fr->end = n;
		fr->index = 0;
		return 0;
	}
}

int fr_read_byte(fd_reader* fr, char* c){
    /* if buffer is used up, re-read */
	if(fr->index >= fr->end){
        /* if has had eof */
		if(fr->end < FR_BUF_SIZE) {
			*c = 0;
			return -1;
		}
		else{
			int n = recv(fr->fd, fr->buf, FR_BUF_SIZE, 0);
			if(n < 0){
				/* error */
				LOG_ERR("fr_read_byte failed: fd read error");
				return -2;
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
		*c = fr->buf[fr->index];
		fr->index++;
		return 0;
	}
}

int fr_read_n(fd_reader* fr, char* c, int n) {
    for (int i = 0; i < n; i++) {
        if(fr_read_byte(fr, c + i) < 0)
            return -1;
    }
    return 0; 
}

void make_fd_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL);
	if(flags < 0) {
		perror("fcntl get");
		exit(-1);
	}
	if(fcntl(fd, F_SETFL, O_NONBLOCK | flags) < 0) {
		perror("fcntl set");
		exit(-1);
	}
}
