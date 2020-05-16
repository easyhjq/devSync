/*
 * Copyright (C) Reage
 * blog:http://www.ireage.com
 */
#include "ds_daemon.h"

static int ds_init_daemon(http_conf *conf)
{
    int fd;

    if (socket_listen_test("127.0.0.1", conf->port))
    {
        exit(0);
    }
    //打开文件/dev/null,使得其拥有守护进程的0，1，2。这样防止守护进程在终端设备上显示输出
    fd = open("/dev/null", O_RDWR);
    if (fd == -1)
    {
        printf("open(\"/dev/null\") failed" DS_LINEEND);
        exit(0);
    }
    //为了保存正可以输出错误信息，标准输出放到最后的

    if (dup2(fd, STDIN_FILENO) == -1)
    {
        printf("dup2(STDIN) failed" DS_LINEEND);
        exit(0);
    }

    if (dup2(fd, STDERR_FILENO) == -1)
    {
        printf("dup2(STDERR) failed" DS_LINEEND);
        exit(0);
    }

    if (dup2(fd, STDOUT_FILENO) == -1)
    {
        printf("dup2(STDOUT) failed" DS_LINEEND);
        exit(0);
    }

    switch (fork())
    {
    case -1:
        printf("fork failed!" DS_LINEEND);
        break;
    case 0:
        break;
    default:
        exit(0);
        break;
    }
}

static ds_init_children(http_conf *conf)
{
    int forStep = 0; //0标识运行accept和cgi， 1 标识只允许faccept, 2fork cgi进程
    //使用两个pipe 完成accept 和cgi进程的通信,pipe 0read, 1write
    int pipHttp[2], pipCgi[2];

    pipe(pipHttp);
    pipe(pipCgi);

    int status;
    while (1)
    {

        if (forStep == 1 || forStep == 0)
        {
            http_children_pid = fork();

            if (http_children_pid == 0)
            {
                cgi_children_pid = 0; //在http进程中不需要杀死兄弟进程cgi
                conf->child_pip.in = pipHttp[0];
                conf->child_pip.out = pipCgi[1];
                close(pipHttp[1]);
                close(pipCgi[0]);
                conf->work_mode = FORK_PROCESS_WORK_HTTP_MODE;
                break;
            }
        }

        if (forStep == 2 || forStep == 0)
        {
            cgi_children_pid = fork();
            if (cgi_children_pid == 0)
            {
                http_children_pid = 0; //在cgi进程中不需要杀死兄弟进程http
                conf->child_pip.in = pipCgi[0];
                conf->child_pip.out = pipHttp[1];
                close(pipHttp[0]);
                close(pipCgi[1]);
                conf->work_mode = FORK_PROCESS_WORK_CGI_MODE;
                break;
            }
        }

        int pid = wait(&status);
        if (pid == http_children_pid)
        {
            forStep = 1;
        }
        else if (pid == cgi_children_pid)
        {
            forStep = 2;
        }
    }
}

int ds_daemon(http_conf *conf, int t)
{
    int uid = getuid();
    int gid = getgid();
    int status = 0;

    handle_signal_init();

    if (conf->user && strlen(conf->user))
    {
        struct passwd *pw = getpwnam(conf->user);
        if (!pw)
        {
            printf(" user[%s] no found\n", conf->user);
            exit(0);
        }
        uid = pw->pw_uid;
        gid = pw->pw_gid;
        printf("\n user:%s\n", conf->user);
    }

    if (t == 0)
    {
        ds_init_daemon(conf);
    }

    if (t == 0 || t == 1)
    {
        ds_init_children(conf);

        //ds_pid = getpid();

        if (setsid() == -1)
        { //setsid创建一个新会话
            printf("setsid() failed!" DS_LINEEND);
            exit(0);
        }

        umask(0);
        setuid(uid);
        setgid(gid);
        ds_init(conf);
    }

    /*if( t == 2) {
       conf->work_mode = FORK_PROCESS_WORK_HTTP_MODE;
   }*/
    if (t == 2)
    {
        //使用两个pipe 完成accept 和cgi进程的通信,pipe 0read, 1write
        int pipHttp[2], pipCgi[2];
        pipe(pipHttp);
        pipe(pipCgi);
        cgi_children_pid = fork();
        if (cgi_children_pid == 0)
        {
            conf->child_pip.in = pipCgi[0];
            conf->child_pip.out = pipHttp[1];
            close(pipHttp[0]);
            close(pipCgi[1]);
            conf->work_mode = FORK_PROCESS_WORK_CGI_MODE;
        }
        else
        {
            conf->child_pip.in = pipHttp[0];
            conf->child_pip.out = pipCgi[1];
            close(pipHttp[1]);
            close(pipCgi[0]);
            conf->work_mode = FORK_PROCESS_WORK_HTTP_MODE;
        }
    }

    return OK;
}

void signal_exit(int no)
{
    int pid = getpid();

    if (1 < cgi_children_pid)
    {
        kill(cgi_children_pid, SIGTERM);
    }
    if (1 < http_children_pid)
    {
        kill(http_children_pid, SIGTERM);
    }
    exit(0);
}

void handle_signal_init()
{
    signal(SIGTERM, signal_exit); //* kill
    signal(SIGINT, signal_exit);  //按下Ctrl-C得到的结果
    signal(SIGQUIT, signal_exit); //按下Ctrl-得到的结果
    signal(SIGSEGV, signal_exit); //段错误
}
