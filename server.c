#include "server.h"

static timer_queue* tq;

int server_run(server_conf* conf) {
	int port = conf->port;
	int nth = conf->nth;

	// timer
	tq = (timer_queue*)malloc(sizeof(timer_queue));
	timer_queue_init(tq);

	/* create listening socket of server */
	int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		perror("create socket");
		close(listen_fd);
		exit(-1);
	}

	/* get sockaddr of server */
	struct sockaddr_in sin_server;
	server_set_sin_server(&sin_server, port);
	int sin_server_len = sizeof(sin_server);
	int optval = 1;
	if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
		perror("fail to set sock opt");
		exit(-1);
	}

	/* bind listen sock and sin_server */
	if(bind(listen_fd, (const struct sockaddr*)&sin_server, sizeof(sin_server)) < 0) {
		perror("fail to bind");
		exit(-1);
	}
	/* listen */
	if (listen(listen_fd, LISTENQ) < 0) {
		perror("listen");
		close(listen_fd);
		exit(-1);
	}
	LOG_INFO("Listening at port %d\n", port);

	/* create a threadpool */
	threadpool* tp = threadpool_create(nth);

	/* epoll */
	int epfd = epoll_create(1024);
	if(epfd < 0) {
		perror("epoll create");
		exit(-1);
	}

	/* epoll - add event of listen_fd */
	make_fd_nonblocking(listen_fd);
	http_request_t tmp;
	tmp.fd = listen_fd;
	struct epoll_event event;
	event.events = EPOLLIN | EPOLLET;
	event.data.ptr = &tmp;
	/* epoll - add event */
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &event) < 0) {
		perror("epoll add listen_fd");
		exit(-1);
	}

	/* epoll - buffer for waiting */
	struct epoll_event triggered_events[100];

	/* main loop */
	while(1){
		/* epoll - wait */	
		//LOG_DEBUG("epoll waiting");
		timer_queue_clean(tq);
		int ntriggered = epoll_wait(epfd, triggered_events, 100, 200);
		if(ntriggered < 0) {
			perror("epoll wait");
			exit(-1);
		}
		else {
			//LOG_DEBUG("epoll: %d fd(s) triggered", ntriggered);
			/* for each triggered fd */
			for(int i = 0; i < ntriggered; i++){
				int fd = ((http_request_t*)(triggered_events[i].data.ptr))->fd;

				/* if trigger listen_fd, accept all and add accept_fd to epoll */
				if(fd == listen_fd){
					/* get sockaddr of client, used to accept*/
					struct sockaddr_in sin_client;
					server_set_sin_client(&sin_client);
					int sin_client_len = sizeof(sin_client);

					/* epoll event for accept_fd */
					struct epoll_event event;
					event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

					/* while loop to accept all connect request */
					while(1) {
						/* accept and add to epoll */
						int accept_fd = accept(listen_fd, (struct sockaddr* restrict)&sin_client, (socklen_t* restrict)&sin_client_len);
						if (accept_fd < 0) {
							if(errno == EAGAIN || errno == EWOULDBLOCK){
								LOG_DEBUG("All requests accepted");
								break; 
							}
							else {
								LOG_ERR("fail to accept");
								break;
							}
						}
						/* set data of epoll_event of accept_fd */
						event.data.ptr = (http_request_t*)malloc(sizeof(http_request_t)); /* free in do_request */
						if(event.data.ptr == NULL) {
							perror("malloc");
							exit(-1);
						}
						http_init_request((http_request_t*)event.data.ptr, epfd, accept_fd);
						make_fd_nonblocking(accept_fd);

						/* timer */
						if(timer_queue_add(tq, ((http_request_t*)event.data.ptr)->tn) < 0) {
							free(((http_request_t*)event.data.ptr)->tn);
							((http_request_t*)event.data.ptr)->tn = NULL;
							LOG_ERR("fail to add timer");
						}
						if(epoll_ctl(epfd, EPOLL_CTL_ADD, accept_fd, &event) < 0) {
							LOG_ERR("err epoll add");
							break;
						}

						LOG_INFO("accepted a connect [%d]", accept_fd);
					}
				}
				/* if not listen_fd */
				else {
					/* make a task and add it to threadpool */
					tp_task* task = threadpool_create_tp_task(server_handle_request, (void*)triggered_events[i].data.ptr);
					threadpool_addtask(tp, task);
				}
			} /* end of for each triggered */
		} /* end of if(trigger 0)else */
	} /* end of main loop */
	
	/* not supposed to be here */
	perror("fatal error");
	exit(-1);
}

int http_handle_get_1_0(int sock_fd, const char* filename, http_request_t* req) {
	FILE* fp = fopen(filename, "r");
	char *buf = (char*)malloc(256);
	
	if(fp == NULL) {
		LOG_ERR("fail to open file [%s], responding 404", filename);
		strcpy(buf, rh_status_404_nl);
		strcat(buf, rh_server_nl);
		strcat(buf, rh_content_type);
		strcat(buf, rh_content_type_html);
		strcat(buf, rh_nl);
		strcat(buf, rh_keepalive_nl);

		strcat(buf, rh_nl);
		if(send(sock_fd, buf, strlen(buf), 0) < 0){
			LOG_ERR("fail to send http response head");;
		}
		else {
			FILE* f404 = fopen(RESPONSE_404_HTML, "r");
			if(f404 == NULL) {
				LOG_ERR("fail to open file [%s]", RESPONSE_404_HTML);
			}
			else {
				int nsend = send_file(f404, sock_fd);
				if(nsend < 0) 
					LOG_ERR("fail to send file [%s]", filename);
				else
					LOG_INFO("404 sent");
				fclose(f404);						
			} /* end of f404 != NULL */
		} /* end of send head succee d */
	} /* end of fp == NULL */
	else{
		/* succeed to open file */					
		strcpy(buf, rh_status_200_nl);
		strcat(buf, rh_server_nl);
		strcat(buf, rh_content_type);
		char* filetype = get_content_type((char*)filename);
		if(filetype != NULL)
			strcat(buf, filetype);
		strcat(buf, rh_nl);
		strcat(buf, rh_nl);
		//LOG_DEBUG("[buf content:]\n%s", buf);

		int nsend = send(sock_fd, buf, strlen(buf), 0);
		if(nsend < 0)
			LOG_ERR("fail to send http response head");
		else {
			nsend = send_file(fp, sock_fd);
			if(nsend < 0) 
				LOG_ERR("fail to send file [%s]", filename);
			else
				LOG_INFO("file sent [%s] [size = %d]", filename, nsend);
		}
		fclose(fp);
	}
	free(buf);
	return 0;
}

void* server_handle_request(void* arg){
	LOG_DEBUG("handling request");
	http_request_t* req = (http_request_t*)arg;
	int fd = req->fd;

	if(req->tn == NULL) {
		/* could not keep alive */
	}
	else {
		/* delete this timer_node (mark as deleted) */
		int del = timer_queue_del_node(req->tn);
	}

	/* parse request */
	LOG_DEBUG("Parsing http request");
	int ret = http_parse_request(req);
	if(ret  < 0){ 
		close(fd);
		free(req);
		LOG_ERR("fail to parse http request");
		return NULL;
	}
	else {
		LOG_DEBUG("succeed to parse http request [%d %s %d]", req->method, req->uri, req->version);
	}

	/* get filename from uri */
	char* filename;
	get_resource_path(&filename, req->uri);

	if(req->version == http_v_1_0 ){
		switch(req->method) {
			case http_m_get:
			{
				http_handle_get_1_0(fd, filename, req);
				break;
			}
			case http_m_post:
			{
				LOG_ERR("Unimplemented HTTP version [%d]", req->method);
			}
			default: 
			{	
				// 400: bad request
				LOG_ERR("Unimplemented HTTP version [%sd]", req->method);
			}
		} // end of switch method
	} 
	else if(req->version == http_v_1_1){
		switch(req->method) {
			case http_m_get:
			{
				http_handle_get_1_0(fd, filename, req);
				break;
			}
			case http_m_post:
			{
				LOG_ERR("Unimplemented HTTP version [%d]", req->method);
			}
			default: 
			{	
				// 400: bad request
				LOG_ERR("Unimplemented HTTP version [%s]", req->method);
			}
		} // end of switch method
	}
	else {
		// 505: http version not supported
		LOG_ERR("Unimplemented HTTP version [%s]", req->version);
	}

	/* after handling */
	free(filename);
	free(req->uri);
	req->uri = NULL;

	if(req->tn == NULL || req->connection == 0) {
		close(fd);
		free(req);
	}
	/* connection: keep-alive */
	else {
		/* reset epoll oneshot */
		struct epoll_event event;
		event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
		event.data.ptr = arg;
		epoll_ctl(req->epfd, EPOLL_CTL_MOD, req->fd, &event);
		
		/* reset timer */
		timer_node* tn = (timer_node*)malloc(sizeof(timer_node));
		tn->deleted = 0;;
		tn->req = req;
		gettimeofday(&tn->t, NULL);
		req->tn = tn;
		timer_queue_add(tq, tn);
	}
	
	LOG_DEBUG("a task finishing");
	return NULL;
}

void server_conf_init(server_conf* conf) {
	conf->port = DEFAULT_PORT;
	conf->nth = DEFAULT_NTH;
}

void server_set_sin_server(sockaddr_in* sin, int port){
	memset(sin, 0, sizeof(sockaddr_in));
	sin->sin_addr.s_addr = htonl(INADDR_ANY);
	sin->sin_family = AF_INET;
	sin->sin_port = htons(port);
}

void server_set_sin_client(sockaddr_in* sin) {
	memset(sin, 0, sizeof(sockaddr_in));
	sin->sin_addr.s_addr = htonl(INADDR_ANY);
	sin->sin_family = AF_INET;
	sin->sin_port = htons(0);
}
