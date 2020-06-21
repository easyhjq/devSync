/*
 * Copyright (C) Reage
 * blog:http://www.ireage.com
 */
#ifndef _HTTP_BASE_H_
#define _HTTP_BASE_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include "buffer.h"
#include "str.h"
#include "pool.h"
#include "hash.h"
#include "http_mod_connect.h"

#define OK 0
#define FILE_NO_EXIST 1
#define FILE_NO_ACCESS 2
#define CANCEL 2
#define DONE -1	   //end
#define CONTINUE 1 //等到资源就绪后，继续处理
#define NEXT 0

#define MEMERROR -1
#define UNDEFINED -144

#define MAX_CONNECT 10000
#define MAX_EVENT 800

#define HTTP_OK 200

#define HTTP_BAD_REQUEST 400
#define HTTP_UNAUTHORIZED 401
#define HTTP_NOT_FOUND 404
#define HTTP_REQUEST_TIMEOUT 408
#define HTTP_NOT_IMPLEMENTED 501
#define HTTP_BAD_GATEWAY 502
#define HTTP_UNDEFINED 544
#define HTTP_WRITE_FILE_FAIL 406

#define HANDLE_STATUS_OK 0
#define HANDLE_STATUS_CONTINUE 1
#define HANDLE_STATUS_FAILD -1

#define FORK_PROCESS_WORK_HTTP_MODE 1
#define FORK_PROCESS_WORK_CGI_MODE 2

typedef enum
{
	_GET,
	_POST,
	_PUT,
	_DEL,
	_SERVICE,
	_CGI,
	_NONE
} http_method_t;

typedef enum
{
	_NOCOMPRESS,
	_GZIP,
	_DEFLATE
} COMPRESS_TYPE;

typedef enum
{
	CGI_STATUS_RUN,
	CGI_STATUS_CLOSEING,
	CGI_STATUS_END
} CGI_STATUS;

typedef enum
{
	READ_HEAD,
	HEAD_PRASE,
	HANDLE,
	COMPRESS,
	WRITE,
} step;

typedef enum
{
	t_char,
	t_uchar,
	t_int,
	t_uint,
	t_long,
} value_type;

typedef struct key
{
	char *name;
	char *value;
	value_type type;
	struct key *next;
} key;

typedef struct fileinfo
{
	string *name;
	size_t len;
	FILE *fp;
	void *start;
} fileinfo_t;

typedef struct mananger_con
{
	struct mananger_con *left;
	struct mananger_con *right;
	void *val;
	struct epoll_event *ev;
	int type;
	int time;
} mananger_con;

typedef struct web_conf
{
	char *root;
	char *index_file;
	int index_count;
	char *err404;
	char *server;
	int fd;
	struct web_conf *next;
} web_conf;

typedef struct fork_child_connect_pipe
{
	int in;
	int out;
} fork_child_connect_pipe_t; //用于父子进程之间通信

typedef struct http_conf
{
	int web_count;
	int port;
	int epfd;
	int fd;
	char *user;
	char *auth;
	fork_child_connect_pipe_t child_pip;
	int work_mode;
	key *mimetype;
	web_conf *web;
} http_conf;

typedef struct response
{
	size_t status_code;
	size_t content_length;

	buffer *server;
	buffer *date;
	buffer **www_authenticate;
	buffer *content_type;
	buffer *physical_path;
	COMPRESS_TYPE content_encoding;
} response;

typedef struct request
{
	string *uri;
	string *host;
	string *args;
	string *clientIp;

	string *rawAuthorization;
	string *auth;
	uint ts;
	http_method_t http_method;
	string *http_version;
	COMPRESS_TYPE accept_encoding;
	unsigned int content_length;
	string *execute_file;
	buffer *header;

	buffer *physicalURI;
	// sync file time
	long int ctime;
	long int atime;
	long int mtime;

} request;

typedef struct http_connect
{
	request *in;
	response *out;
	fileinfo_t write_file;
	web_conf *web;
	pool_t *p;
	// 是否启用auth, = NULL 没有启用，否则启动，里面是usr:pwd,有点坑
	string *auth;
	int fd;
	int (*next_handle)(struct http_connect *con);
} http_connect_t;

typedef struct epoll_extra_data
{
	int type;
	void *ptr;
} epoll_extra_data_t;

typedef struct cgi_ev_t
{
	char *ev[20];
	int count;
	int stdin;
	int stdout;

} cgi_ev_t;

typedef struct epoll_cgi
{
	string *file;
	pool_t *p;
	struct fork_child_connect_pipe pipe;
	unsigned int pid;
	unsigned int last_run_ts;
	unsigned int last_add_ts;
	string *outfile;
	int fd;
	CGI_STATUS status; //0运行中，1开始回收，2运行结束
	list_buffer_t *cgi_data;
	//list_buffer_t *out;
} epoll_cgi_t;

typedef struct execute_cgi_info_manager
{
	hash_t *h;
	pool_t *p; //没有写内存回收策略，在hash的bucket多次改变，会出现浪费内存，
} execute_cgi_info_manager_t;

#define _Server "DevSync"
#define _Auth_desc "DevSync"
#define _Version "0.2.2"
#define DS_LINEEND "\x0a"

#endif
