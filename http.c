 #include "http.h"

int http_request_head_parse_0(int fd, http_request_t* req){
	/* a utility to read from fd */
	file_reader* fr = (file_reader*)malloc(sizeof(file_reader));
	int fr_ret = fr_init(fr, fd);
	if(fr_ret < 0) {
		log_err("fr_init failed: fd error");
		return -2;
	}
	/* if state changed after reading, then don't need read again */
	int need_read = 1;

	/* an FSM to parse http request [method][url][version] */
	int state = hp_method;
	/* used to parse method, url and version */
	int len_method, len_url, len_version;
	char c;
	int i = 0;

	/* buffer to store string temporarily */
	char* buffer = (char*)malloc(2048);
	if(buffer == NULL) {
		perror("malloc");
		exit(-1);
	} 

	char *url = NULL;

	while(1){
		if(need_read && fr_read_byte(fr, &c) < 0) {
			/* eof before finishing parsing */
			state = hp_error;
		}

		switch (state) {
		case hp_method:
		{
			if(c == '\r' || c == '\n') {
				need_read = 0;
				/* if any thing wrong, go to state errorm */
				state = hp_error;
			}
			else if(isspace(c)) {
				need_read = 0;
				state = hp_space_1;
				len_method = i;
				buffer[i] = 0;
				req->method = get_http_request_method(buffer);
			}
			else {
				need_read = 1;
				buffer[i] = c;
				i++;
			}
			break;
		}
		case hp_space_1:
		{	
			if(c == '\r' || c == '\n') {
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
		}
		case hp_url:
		{
			if(c == '\r' || c == '\n') {
				need_read = 0;
				state = hp_error;
			}
			else if(isspace(c)) {
				need_read = 0;
				state = hp_space_2;
				len_url = i;
				buffer[i] = 0;
				url = (char*)malloc(i + 1);
				memcpy(url, buffer, i + 1);
			}
			else {
				need_read = 1;
				buffer[i] = c;
				i++;
			}
			break;
		}
		case hp_space_2:
		{
			if(c == '\r' || c == '\n') {
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
		}
		case hp_version:
		{
			if(c == '\r' || c == '\n') {
				len_version = i;
				buffer[i] = 0;
				req->version = get_http_version(buffer);
				need_read = 0;
				state = (c == '\r' ? hp_success_0 : hp_success);
				i = 0;
			}
			else if(isspace(c)) {
				need_read = 0;
				state = hp_error;
			}
			else {
				need_read = 1;
				buffer[i] = c;
				i++;
			}
			break;
		}
		case hp_error:
		{	
			if(url != NULL)
				free(url);
			free(buffer);
			free(fr);
			return -1;
		}
		case hp_success_0:
		{
			if(c == '\r') {
				need_read = 1;
				state = hp_success_0;
			}
			else if(c == '\n') {
				need_read = 0;
				state = hp_success;
			}
			else {
				need_read = 0;
				state = hp_error;
			}
			break;
		}
		case hp_success:
		{
			free(buffer);
			free(fr);
			req->url = url;
			return 0;
		}
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
	const int bufsize = 64;
	char buf[bufsize];
	int nsend = 0;
	while(!feof(fp)) {		
		int nread = fread(buf, 1, bufsize, fp);
		if(nread < 0){
			log_err("fread");
			return -1;
		} 
		/* to avoid shutdown when receiving SIGPIPE */
		int t = send(sock_fd, buf, nread, MSG_NOSIGNAL);
		if(t < 0) {
			log_err("fail to send");
			return -1;
		}
		nsend += t;
	}
	return nsend;
}

http_request_method_t get_http_request_method(const char* method) {
	if(!strncasecmp(method, "GET", 3)) 
		return http_m_get;
	else if(!strncasecmp(method, "POST", 4)) 
		return http_m_post;
	else if(!strncasecmp(method, "HEAD", 4)) 
		return http_m_head;
	else if(!strncasecmp(method, "PUT", 3)) 
		return http_m_put;
	else if(!strncasecmp(method, "DELETE", 6)) 
		return http_m_delete;
	else if(!strncasecmp(method, "CONNECT", 7)) 
		return http_m_connect;
	else if(!strncasecmp(method, "OPTIONS", 7)) 
		return http_m_options;
	else if(!strncasecmp(method, "TRACE", 5)) 
		return http_m_trace;
	else if(!strncasecmp(method, "PATCH", 5)) 
		return http_m_patch;
	else 
		return http_m_err;
	
}

http_version_t get_http_version(const char* version) {
	if(!strncasecmp(version, "HTTP/1.0", 8)) 
		return http_v_1_0;
	else if(!strncasecmp(version, "HTTP/1.1", 8))
		return http_v_1_1;
	else
		return http_v_err;
}

char* get_contene_type(char* filename) {
	char* p = filename;
	char* q = filename;
	while(*p != 0) {
		if(*p == '.')
			q = p + 1;
		p++;
	}
	if(!strcasecmp(q, "png"))
		return rh_content_type_png;
	else if(!strcasecmp(q, "html"))
		return rh_content_type_html;
	else if(!strcasecmp(q, "ico"))
		return rh_content_type_icon;
	else
		return NULL;
}
