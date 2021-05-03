 #include "http.h"

int http_request_head_parse_0(int fd, char** method, char** url, char** version){
	file_reader* fr = (file_reader*)malloc(sizeof(file_reader));
	int fr_ret = fr_init(fr, fd);
	if(fr_ret < 0) {
		log_err("fr_init failed: fd error");
		return -2;
	}

	/* an FSM to parse http request [method][url][version] */
	int len_method, len_url, len_version;
	char c;
	int i = 0;
	char* buffer = (char*)malloc(4096);
	if(buffer == NULL) {
		perror("malloc");
		exit(-1);
	}
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
			break;
		case hp_error:
			free(fr);
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
			break;
		case hp_success:
			free(fr);
			return 0;
		default:
			/* unexpected */
			break;
		}
	}
	perror("http request head parse fatal error");
	exit(-1);
}

void get_resource_path(char** path, char* url){
	int root_dir_len = strlen(RESOURCE_ROOT_DIR);
	int url_len = strlen(url);
	int url_is_dir = 0;
	int len = root_dir_len + url_len;

	/* .../index.html */
	if(url[strlen(url) - 1] == '/') {
		url_is_dir = 1;
		len += 10;
	}

	*path = (char*)malloc(len + 1);
	if(*path == NULL){
		perror("malloc");
		exit(-1);
	}
	strcpy(*path, RESOURCE_ROOT_DIR);
	strcat(*path + root_dir_len, url);
	if(url_is_dir)
		strcat(*path + root_dir_len + url_len, "index.html");
}

int send_file(FILE* fp, int sock_fd) {
	char buf[10];
	while(!feof(fp)) {		
		if(fgets(buf, sizeof(buf), fp) == NULL){
			log_err("fgets");
			return -1;
		}
		/* to avoid shutdown when receiving SIGPIPE */
		if(send(sock_fd, buf, strlen(buf), MSG_NOSIGNAL) < 0) {
			log_err("fail to send");
			return -1;
		}
	}
	return 0;
}