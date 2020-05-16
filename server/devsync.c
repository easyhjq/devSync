/*
 * Copyright (C) Reage
 * blog:http://www.ireage.com
 */
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "base.h"
#include "http_request.h"
#include "cgi.h"
#include "modules/ds_daemon.h"

static int ds_version;
static int ds_help;
static int ds_port = 8484;
static int t = 0; //0:daemon,1:children,2:single
static char *auth;
static char *user;

int ds_get_options(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	http_conf conf;
	conf.web = NULL;
	conf.web_count = 0;
	conf.mimetype = NULL;
	conf.user = NULL;
	conf.auth = NULL;

	ds_get_options(argc, argv);

	if (ds_version)
	{
		printf("devSync version: "_Version DS_LINEEND);
		return OK;
	}
	if (ds_help)
	{
		printf("devSync version: "_Version DS_LINEEND);
		printf("Usage: devsync [-vh]" DS_LINEEND
			   "[-p port]" DS_LINEEND
			   "[-u user]" DS_LINEEND
			   "[-t type]" DS_LINEEND
			   "[-a user:pwd]" DS_LINEEND
				   DS_LINEEND
			   "Options:" DS_LINEEND
			   "   -v            : show version and exit" DS_LINEEND
			   "   -h            : this help" DS_LINEEND
			   "   -a            : simple authentication model, base web browser http code 401" DS_LINEEND
				   DS_LINEEND
			   "Descrption:" DS_LINEEND
			   "-t : daemon, children, single" DS_LINEEND
			   "\t daemon: application daemon run, children: application run terminal, handle process is fork; single: run termianal" DS_LINEEND);

		return OK;
	}

	conf.port = ds_port;
	if (user != NULL)
	{
		// 回写进程显示内容，要将argv 的对象拷贝出去
		char cpyUser[512];
		memcpy(cpyUser, user, sizeof(char) * 512);
		conf.user = cpyUser;
	}
	if (auth != NULL)
	{
		// 回写进程显示内容，要将argv 的对象拷贝出去
		char cpyAuth[512];
		memcpy(cpyAuth, auth, sizeof(char) * 512);
		conf.auth = cpyAuth;
	}
	// 回写进程显示内容，要将argv 的对象拷贝出去
	sprintf(argv[0], "devSync master                                                                                  ");

	ds_daemon(&conf, t);

	if (conf.work_mode == FORK_PROCESS_WORK_HTTP_MODE)
	{
		sprintf(argv[0], "devSync HTTP                                    ");
		start_accept(&conf);
	}
	else
	{
		sprintf(argv[0], "devSync CGI                                                                                  ");
		start_cgi(&conf);
	}

	return 0;
}

int ds_get_options(int argc, char *argv[])
{
	int i;
	char *p;

	for (i = 1; i < argc; i++)
	{
		p = (char *)argv[i];

		if (strlen(p) != 2)
		{
			printf("\ninvalid option: \"%s\"\n\n", argv[i]);
			ds_help = 1;
			return -1;
		}
		if (*p++ != '-')
		{
			printf("\n invalid option: \"%s\"\n\n", argv[i]);
			return -1;
		}
		while (*p)
		{
			switch (*p++)
			{
			case 'p':
				if (atoi(argv[++i]) > 0)
				{
					ds_port = atoi(argv[i]);
				}
				break;
			case 'v':
				ds_version = 1;
				break;
			case 'h':
				ds_help = 1;
				break;
			case 'u':
				user = argv[++i];
				break;
			case 'a':
				auth = argv[++i];
				break;
			case 't':
				i++;
				int slen = strlen(argv[i]);
				if (strncmp("daemon", argv[i], slen) == 0)
				{
					t = 0;
				}
				else if (strncmp("children", argv[i], slen) == 0)
				{
					t = 1;
				}
				else if (strncmp("single", argv[i], slen) == 0)
				{
					t = 2;
				}

				break;
			}
		}
	}

	return OK;
}
