 #include "http.h"

int http_init_request(http_request_t* r, int epfd, int fd) {
	r->epfd = epfd;
	r->fd = fd;

	timer_node* tn = (timer_node*)malloc(sizeof(timer_node)); /* free in timer_queue_pop_min */
	if(tn == NULL) {
		perror("malloc");
		exit(-1);
	}
	tn->req = r;
	tn->deleted = 0;
	gettimeofday(&tn->t, NULL);
	r->tn = tn;

	r->method = -1;
	r->uri = NULL;
	r->version = -1;
	r->connection = 0;
	return 0;
}

int http_parse_request(http_request_t* req) {
	/* This function is to parse http request and store in arg req.
	 * An http request is like: 
	 *		GET /xx/xx HTTP/1.1
	 * 		header(key): value
	 * 		header: value
	 * 		...
	 * Need to get method, uri, version, and header/value of each line.
	 */

	/* fr is a utility to read from fd */
	int fd = req->fd;
	fd_reader* fr = (fd_reader*)malloc(sizeof(fd_reader));
	int fr_ret = fr_init(fr, fd);
	if(fr_ret < 0) {
		LOG_ERR("fr_init failed: fd error[%d]", fr_ret);
		free(fr);
		return -2;
	}
	/* if state changed after reading(known by reading one byte), then don't need read again */
	int need_read = 1;

	/* use an FSM to parse http request */
	int state = hp_s_method;
	int fr_end = 0;
	char c = -1;
	int i = 0;
	char *uri = NULL;

	/* buffer to store string temporarily */
	const int bufsize = 4096;
	char* buffer = (char*)malloc(bufsize);
	if(buffer == NULL) {
		perror("malloc");
		exit(-1);
	} 


	while(1) {
		if(need_read && fr_read_byte(fr, &c) < 0) {
			/* if cannot read more */
			//LOG_DEBUG("eof, state = %d", state);
			fr_end = 1;
		}

		switch (state) {
		case hp_s_method:
		{
			if(fr_end) {
				need_read = 0;
				state = hp_s_error;
			}
			else if(c == ' ') {
				need_read = 0;
				state = hp_s_space_1;
				buffer[i] = 0;
				req->method = get_http_request_method(buffer);
			}
			else if(isspace(c) || i >= bufsize) {
				need_read = 0;
				state = hp_s_error;
			}
			else {
				need_read = 1;
				buffer[i] = c;
				i++;
			}
			break;
		}
		case hp_s_space_1:
		{	
			if(fr_end) {
				need_read = 0;
				state = hp_s_error;
			}
			else if(c == ' ') {
				need_read = 1;
			}
			else if (isspace(c)){
				need_read = 0;
				state = hp_s_error;
			}
			else {
				need_read = 0;
				state = hp_s_uri;
				i = 0;
			}
			break;
		}
		case hp_s_uri:
		{
			if(fr_end) {
				need_read = 0;
				state = hp_s_error;
			}
			else if(c == ' ') {
				need_read = 0;
				state = hp_s_space_2;
				buffer[i] = 0;
				uri = (char*)malloc(i + 1);
				req->uri = uri;
				memcpy(uri, buffer, i + 1);
			}
			else if(isspace(c) || i >= bufsize) {
				need_read = 0;
				state = hp_s_error;
			}
			else {
				need_read = 1;
				buffer[i] = c;
				i++;
			}
			break;
		}
		case hp_s_space_2:
		{
			if(fr_end) {
				need_read = 0;
				state = hp_s_error;
			}
			else if(c == ' ') {
				need_read = 1;
			}
			else if(!isspace(c)) {
				need_read = 0;
				state = hp_s_version;
				i = 0;
			}
			else {
				need_read = 0;
				state = hp_s_error;
			}
			break;
		}
		case hp_s_version:
		{
			if(fr_end) {
				need_read = 0;
				state = hp_s_error;
			}
			else if(i < bufsize && c == '\r') {
				buffer[i] = 0;
				req->version = get_http_version(buffer);
				need_read = 0;
				state = hp_s_line_cr;
				i = 0;
			}
			else if(isspace(c) || i >= bufsize) {
				need_read = 0;
				state = hp_s_error;
			}
			else {
				need_read = 1;
				buffer[i] = c;
				i++;
			}
			break;
		}
		case hp_s_error:
		{	
			/* if any thing wrong, go to state error */
			if(uri != NULL)
				free(uri);
			free(buffer);
			free(fr);
			return -1;
		}
		case hp_s_line_cr:
		{
			if(fr_end) {
				need_read = 0;
				state = hp_s_error;
			}
			else if(c == '\r') {
				need_read = 1;
			}
			else if(c == '\n') {
				need_read = 0;
				state = hp_s_line_lf;
			}
			else{
				need_read = 0;
				state = hp_s_error;
			}
			break;
		}
		case hp_s_line_lf:
		{
			if(fr_end) {
				need_read = 0;
				state = hp_s_finish;
			}
			else if(c == '\n') {
				need_read = 1;
			}
			else if(!isspace(c)) {
				/* after parsing method, uri and version, parse [key, value] */
				need_read = 0;
				state = hp_s_next_key;
			}
			else{
				need_read = 0;
				state = hp_s_error;
			}
			break;
		}
		case hp_s_key:
		{
			/* read key until ':' */
			/* Key: value */
			/*    ^       */
			if(fr_end) {
				need_read = 0;
				state = hp_s_error;
			}
			else if(i < bufsize && c == ':') {
				need_read = 0;
				buffer[i] = 0;
				state = hp_s_key_end;
			}
			else if(i < bufsize) {
				need_read = 1;
				buffer[i] = c;
				i++;
			}
			else {
				need_read = 0;
				state = hp_s_error;
			}
			break;
		}
		case hp_s_key_end:
		{
			if(fr_end) {
				need_read = 0;
				state = hp_s_error;
			}
			else if (c == ':') {
				/* Key: value */
				/*    ^       */
				need_read = 1;
			}
			else if (c == ' ') {
				/* Key: value */
				/*      ^     */
				char* pkey = buffer;
				char* pval = buffer + i + 1;

				/* get value */
				int r = http_parse_value(fr, pval, bufsize - i - 1);
				if(r < 0) {
					need_read = 0;
					state = hp_s_error;
				}
				else {
					//LOG_DEBUG("parse key/value ok -- [%s]: [%s]", pkey, pval);
					/* set [key/value] */
					http_set_request_value(pkey, pval, req);

					/* next line */
					need_read = 1;
					//LOG_DEBUG("next line1, c = [%d]", c);
					state = hp_s_next_key;
					i = 0;
				}
				
			}
			break;
		}
		case hp_s_finish:
		{
			free(buffer);
			free(fr);
			//LOG_DEBUG("Parse finished");
			return 0;
		}
		case hp_s_next_key:
		{
		/*start of parsing a [key, value]*/
			if(c == '\r') {
				need_read = 1;
			}
			else if(c == '\n') {
				need_read = 1;
				state = hp_s_finish;
			}
			/* has next [key, value] */
			else {
				need_read = 0;
				state = hp_s_key;
			}
			break;
		}
		default:
			/* unexpected */
			break;
		}
	}

	free(buffer);
	free(fr);
	return 0;
}

int http_set_request_value(char* key, char* value, http_request_t* req) {
	if(!strcasecmp(key, "Connection")) {
		if(!strcasecmp(value, "keep-alive")) {
			req->connection = 1;
		}
	}

	return 0;
}

int http_parse_value(fd_reader* fr, char* buf, int bufsize) {
	/* Key: value */
	/*      ^     */
	int state = hp_s_val;
	int need_read = 1;
	char c;
	int i = 0;
	while(1) {
		if(need_read && fr_read_byte(fr, &c) < 0) {
			/* eof before finishing parsing */
			return -1;
		}
		
		switch (state)
		{
		case hp_s_val:
		{
			if(i < bufsize && c == '\r') {
				need_read = 0;
				state = hp_s_line_cr;
				buf[i] = 0;
			}
			else{
				need_read = 1;
				buf[i] = c;
				i++;
			}
			break;
		}
		case hp_s_line_cr:
		{
			if(c == '\r') {
				need_read = 1;
				state = hp_s_line_lf;
			}
			else {
				return -1;
			}
			break;
		}
		case hp_s_line_lf:
		{
			if(c == '\n') {
				/* Key: value\r\n means parse succeeded */
				/*             ^                        */
				return 0;
			}
			break;
		}
		default:
			return -1;
		}

	}
}

void get_resource_path(char** path, char* uri) {
	int root_dir_len = strlen(RESOURCE_ROOT_DIR);
	int url_len = strlen(uri);
	int url_is_dir = 0;
	int len = root_dir_len + url_len;

	/* .../index.html */
	if(uri[strlen(uri) - 1] == '/') {
		url_is_dir = 1;
		len += 10;
	}

	*path = (char*)malloc(len + 1);
	if(*path == NULL){
		perror("malloc");
		exit(-1);
	}
	strcpy(*path, RESOURCE_ROOT_DIR);
	strcat(*path + root_dir_len, uri);
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
			LOG_ERR("fread");
			return -1;
		} 
		/* to avoid shutdown when receiving SIGPIPE */
		int t = send(sock_fd, buf, nread, MSG_NOSIGNAL);
		if(t < 0) {
			LOG_ERR("fail to send");
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

char* get_content_type(char* filename) {
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

