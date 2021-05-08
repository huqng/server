#include "server.h"

void server_conf_init(server_conf* conf) {
	conf->port = DEFAULT_PORT;
	conf->use_epoll = 0;
	conf->nth = DEFAULT_NTH;
}

void server_conf_set_port(server_conf* conf, int port) {
	conf->port = port;
}

void server_conf_set_epoll(server_conf* conf, int use_epoll) {
	conf->use_epoll = use_epoll;
}

void set_sin_server(sockaddr_in* sin, int port){
	memset(sin, 0, sizeof(sockaddr_in));
	sin->sin_addr.s_addr = htonl(INADDR_ANY);
	sin->sin_family = AF_INET;
	sin->sin_port = htons(port);
}

void set_sin_client(sockaddr_in* sin) {
	memset(sin, 0, sizeof(sockaddr_in));
	sin->sin_addr.s_addr = htonl(INADDR_ANY);
	sin->sin_family = AF_INET;
	sin->sin_port = htons(0);
}

int run_server(server_conf* conf) {
	int port = conf->port;
	int use_epoll = conf->use_epoll;
	int nth = conf->nth;


	/* create listening socket of server */
	int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		perror("create socket");
		close(listen_fd);
		exit(-1);
	}

	/* get sockaddr of server */
	struct sockaddr_in sin_server;
	set_sin_server(&sin_server, port);
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
	printf("Listening at port %d\n", port);

	/* create a threadpool */
	threadpool* pool = threadpool_create(nth);

	if(use_epoll) {

		printf("using epoll\n");
		/* epoll */
		int epfd = epoll_create(1024);
		if(epfd < 0) {
			perror("epoll create");
			exit(-1);
		}

		/* epoll - set event */
		struct epoll_event event;
		event.events = EPOLLIN | EPOLLET;
		event.data.fd = listen_fd;

		make_fd_nonblocking(listen_fd);

		/* epoll - add event */
		if(epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &event) < 0) {
			perror("epoll add listen_fd");
			exit(-1);
		}

		/* epoll - buffer for waiting */
		struct epoll_event triggered_events[100];

		while(1){
			/* epoll - wait */	
			log_info("epoll waiting");
			int ntriggered = epoll_wait(epfd, triggered_events, 100, -1);
			if(ntriggered < 0) {
				perror("epoll wait");
				exit(-1);
			}
			else{
				log_info("epoll: %d fd(s) triggered", ntriggered);
				/* for each triggered fd */
				for(int i = 0; i < ntriggered; i++){
					/* if trigger listen_fd, accept all and add accept_fd to epoll */
					if(triggered_events[i].data.fd == listen_fd){
						/* get sockaddr of client */
						struct sockaddr_in sin_client;
						set_sin_client(&sin_client);
						int sin_client_len = sizeof(sin_client);

						/* epoll event for accept_fd */
						struct epoll_event event;
						event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
						event.data.fd = -1;

						/* while loop to accept all connect request */
						while(1) {
							/* accept and add to epoll */
							int accept_fd = accept(listen_fd, (struct sockaddr* restrict)&sin_client, (socklen_t* restrict)&sin_client_len);
							if (accept_fd < 0) {
								if(errno == EAGAIN || errno == EWOULDBLOCK){
									log_info("All requests accepted");
									break; 
								}
								else {
									//log_err("accept");
									log_info("fail to accept");
									break;
								}
							}
							event.data.fd = accept_fd;
							make_fd_nonblocking(accept_fd);
							if(epoll_ctl(epfd, EPOLL_CTL_ADD, accept_fd, &event) < 0) {
								log_err("epoll add");
								break;
							}

							log_info("accepted a connect");
						}
					}
					/* if not listen_fd */
					else {
						/* make a task and assign it to threadpool */
						int *fd = (int*)malloc(sizeof(int));
						*fd = triggered_events[i].data.fd;
						tp_task* task = threadpool_create_tp_task(handle_request, fd);
						threadpool_addtask(pool, task);
					}
				}
			}
		}
	}
	else {


		/* listening */
		while (1){
			/* get sockaddr of client */
			struct sockaddr_in sin_client;
			set_sin_client(&sin_client);
			int sin_client_len = sizeof(sin_client);
			
			/* accept requests from client & get a new socket */
			int accept_sock = accept(listen_fd, (struct sockaddr* restrict)&sin_client, (socklen_t* restrict)&sin_client_len);
			if (accept_sock < 0) {
				log_err("fail to accept")
				continue;
				//perror("accept");
				//exit(-1);
			}
			log_info("accepted a connect");

			/* make a task and assign it to threadpool */
			int* fd = (int*)malloc(sizeof(int));
			*fd = accept_sock;
			tp_task* task = threadpool_create_tp_task(handle_request, fd);
			threadpool_addtask(pool, task);
		}
	}

	perror("fatal error");
	threadpool_destroy(pool);
	return 0;


}

void* handle_request(void* arg){
	log_info("handling");
	int sock_fd = *(int*)arg;
	http_request_t req;
	/* get method, url and version */
	if(http_request_head_parse_0(sock_fd, &req) < 0){
		log_err("fail to parse http request head");
		return NULL;
	}
	else
		log_info("parse: [%s] [%s] [%s]", method, url, version);
 
	char* filename;
	get_resource_path(&filename, req.url);

	if(req.version == http_v_1_0 || req.version == http_v_1_1){
		switch(req.method) {
			case http_m_get:
			{
				FILE* fp = fopen(filename, "r");
				
				if(fp == NULL) {
					log_err("fail to open file [%s]", filename);
					// TODO - 404
				}
				else{
					/* succeed to open file */
					char head[] = 
						"HTTP/1.1 200 OK\r\n"
						"Server: Nonserver\r\n"
						"Content-Type: text/html\r\n\r\n";
					if(send(sock_fd, head, strlen(head), 0) < 0){
						log_err("fail to send http response head");;
					}
					else if(send_file(fp, sock_fd) < 0) {
						log_err("fail to send file [%s]", filename);
					}
					else
						log_info("file sent [%s]", filename);

					fclose(fp);
				}
				break;
			}
			case http_m_post:
			{
				log_err("Unimplemented HTTP version [%d]", req.method);
			}
			default: 
			{	
				// 400: bad request
				log_err("Unimplemented HTTP version [%sd]", req.method);
			}
		} // end of switch method
	} 
	else{
		// 505: http version not supported
		log_err("Unimplemented HTTP version [%s]", req.version);
	}

	close(sock_fd);
	free(filename);
	free(req.url);
	log_debug("a task finishing");
	return NULL;
}

