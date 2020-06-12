/*
 * Copyright (C) Reage
 * blog:http://www.ireage.com
 */
#include "http_request.h"

response *response_init(pool_t *p)
{
	response *out;

	out = palloc(p, sizeof(response));
	out->status_code = HTTP_UNDEFINED;
	out->content_length = 0;
	out->server = NULL;
	out->date = NULL;
	out->www_authenticate = NULL;
	out->content_type = NULL;
	out->physical_path = NULL;
	out->content_encoding = _NOCOMPRESS;

	return out;
}

request *request_init(pool_t *p)
{
	request *in;

	in = pcalloc(p, sizeof(request));
	in->execute_file = NULL;

	return in;
}

static int start_web_server(http_conf *g)
{
	int count;
	int epfd;
	int fd = 0;
	epoll_extra_data_t *data;

	data = (epoll_extra_data_t *)malloc(sizeof(epoll_extra_data_t));

	data->type = SERVERFD;
	if (g->port <= 0)
		g->port = 80;
	epfd = epoll_init(MAX_CONNECT);
	//while(count--){
	fd = socket_listen("127.0.0.1", g->port);
	//	web->fd = fd;
	epoll_add_fd(epfd, fd, EPOLL_R, data);
	//	web = web->next;
	//}

	g->fd = fd;
	g->epfd = epfd;

	return epfd;
}

void client_request_close()
{
	char msg[] = "pip error, exit(1)\n";
	write(STDOUT_FILENO, msg, strlen(msg));
	exit(1);
}

void handle_request_destory(http_conf *g, struct epoll_event *evfd)
{
	http_connect_t *con;
	epoll_data_t *epoll_data = (epoll_data_t *)evfd->data.ptr;

	con = (http_connect_t *)epoll_data->ptr;
	if (con->in != NULL)
	{
		ds_log(con, "  [END] ", LOG_LEVEL_DEFAULT);
		epoll_edit_fd(g->epfd, evfd, EPOLL_W);

		pool_destroy(con->p);
		if (con->fd != 0)
		{
			close(con->fd);
		}
	}
	con->next_handle = NULL;
	epoll_del_fd(g->epfd, evfd);
}

void handle_request_socket(http_conf *g, struct epoll_event *evfd)
{

	epoll_extra_data_t *epoll_data = (epoll_extra_data_t *)evfd->data.ptr;

	http_connect_t *con = (http_connect_t *)epoll_data->ptr;
	if (g->auth != NULL)
	{
		con->auth = string_init_from_str(con->p, g->auth, strlen(g->auth));
	}
	if (con->in == NULL)
	{
		//accept_handler(g, con, evfd);
		epoll_edit_fd(g->epfd, evfd, EPOLL_W);
		return;
		//epoll_del_fd(g->epfd, evfd);
	}
	while (con->next_handle != NULL)
	{
		int ret = con->next_handle(con);
		if (ret == DONE)
		{
			if (con->in->execute_file != NULL && con->in->execute_file->len > 0)
			{
				char *shPath = (char *)palloc(con->p, sizeof(char) * con->in->execute_file->len + 1);
				strncpy(shPath, con->in->execute_file->ptr, con->in->execute_file->len);
				ds_log(con, " [send execute sh command:]", LOG_LEVEL_DEFAULT);
				send_execute_sh_cmd(con, g);
			}
			/*else if (con->in->http_method == _DEL)
			{
				ds_log(con, " [send del sh command:]", LOG_LEVEL_DEFAULT);
				send_execute_sh_cmd(con, g);
			}*/
			con->next_handle = NULL;
			epoll_del_fd(g->epfd, evfd);
			close(con->fd);
			ds_log(con, "  [END] ", LOG_LEVEL_DEFAULT);
			pool_destroy(con->p);
			//handle_request_destory(g, evfd);
		}
		else if (ret == CONTINUE)
		{
			break;
		}
	}
}

void accept_request_socket(http_conf *g)
{
	int confd;
	struct sockaddr addr;
	struct sockaddr_in addrIn;
	socklen_t addLen = sizeof(struct sockaddr);

	while ((confd = accept(g->fd, &addr, &addLen)) && confd > 0)
	{
		pool_t *p = (pool_t *)pool_create();
		http_connect_t *con;
		epoll_extra_data_t *data_ptr;

		addrIn = *((struct sockaddr_in *)&addr);
		data_ptr = (epoll_extra_data_t *)palloc(p, sizeof(epoll_extra_data_t));
		con = (http_connect_t *)palloc(p, sizeof(http_connect_t)); //换成初始化函数，
		con->p = p;
		con->fd = confd;
		con->in = (request *)request_init(p);
		con->out = (response *)response_init(p);
		if (g->auth != NULL)
		{
			con->auth = string_init_from_str(p, g->auth, strlen(g->auth));
		}
		char *ip = NULL;
		if (addrIn.sin_addr.s_addr)
		{
			ip = inet_ntoa(addrIn.sin_addr);
		}

		if (ip)
		{
			con->in->clientIp = (string *)string_init_from_str(p, ip, strlen(ip));
		}

		make_fd_non_blocking(confd);
		con->next_handle = read_header;
		data_ptr->type = SOCKFD;
		data_ptr->ptr = (void *)con;
		epoll_add_fd(g->epfd, confd, EPOLL_R, (void *)data_ptr); //对epoll data结构指向的结构体重新封装，分websit
																 //data struct ,  connect  data struct , file data struct ,
	}
}

void destory_connect(http_conf *g, http_connect_t *con, struct epoll_event *evfd)
{
	if (con->in != NULL)
	{
		close(con->fd);
		ds_log(con, "  [END] ", LOG_LEVEL_DEFAULT);
		epoll_edit_fd(g->epfd, evfd, EPOLL_W);

		pool_destroy(con->p);
	}
	con->next_handle = NULL;
	epoll_del_fd(g->epfd, evfd);
}

int start_accept(http_conf *g)
{
	int count;
	int confd;
	struct epoll_event ev[MAX_EVENT];
	struct epoll_event *evfd;
	epoll_extra_data_t *epoll_data;
	struct sockaddr addr;
	struct sockaddr_in addrIn;
	socklen_t addLen = sizeof(struct sockaddr);
	int evIndex;

	start_web_server(g);

	//处理write pipe 错误。向已经关闭的socket fd 写内容触发
	signal(SIGPIPE, client_request_close);

	printf("--------------- start server\n--------------");
	while (1)
	{
		count = epoll_wait(g->epfd, ev, MAX_EVENT, -1);

		accept_request_socket(g);
		if (count < 0)
		{
			count = 0;
		}

		evfd = ev;
		for (evIndex = 0; evIndex < count; evIndex++)
		{
			epoll_data = (epoll_extra_data_t *)evfd->data.ptr;
			if ((evfd->events & EPOLLIN))
			{

				switch (epoll_data->type)
				{
				case SOCKFD:
					handle_request_socket(g, evfd);
					/*con = (http_connect_t *) epoll_data->ptr;
					
						while(con->next_handle != NULL) {
							int ret = con->next_handle(con);
							if(ret == DONE) {
								if(con->in->execute_file != NULL && con->in->execute_file->len > 0) {
									char *shPath = (char *)palloc(con->p, sizeof(char)*con->in->execute_file->len +1);
									strncpy(shPath, con->in->execute_file->ptr, con->in->execute_file->len);
									ds_log(con, " [send execute sh command:]", LOG_LEVEL_DEFAULT);
									send_execute_sh_cmd(con, g);
								}
								destory_connect(g, con, evfd);
								//con->next_handle = NULL;
								//epoll_del_fd(g->epfd, evfd);
								//close(con->fd);
								//ds_log(con, "  [END] ", LOG_LEVEL_DEFAULT);
								//pool_destroy(con->p);
							}else if(ret == CONTINUE) {
								break;
							}
						}*/
					break;
				case CGIFD:
				{
					epoll_cgi_t *cgi = (epoll_cgi_t *)epoll_data->ptr;

					break;
				}
				case SERVERFD:
					break;
				}
			}
			else if (evfd->events & EPOLLOUT)
			{
			}
			else if (evfd->events & EPOLLRDHUP)
			{

				http_connect_t *con;

				con = (http_connect_t *)epoll_data->ptr;
				destory_connect(g, con, evfd);

				/*if(con->in != NULL) {
					ds_log(con, "  [END] ", LOG_LEVEL_DEFAULT);
					epoll_edit_fd(g->epfd, evfd, EPOLL_W);
					
					pool_destroy(con->p);
					close(con->fd);

				}
				con->next_handle = NULL;
				epoll_del_fd(g->epfd, evfd);*/
			}

			evfd++;
		}
	}
}
