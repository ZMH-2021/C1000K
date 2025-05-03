

#ifndef __REACTOR_H__
#define __REACTOR_H__

#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>

#define MAX_EVENTS 8192
#define BUFFER_LENGTH 64
#define CONNECTION_SIZE 1000000
#define MAX_PORTS 1024
#define TIME_PRINT 10000
#define TIME_SUB_MS(tv1, tv2) ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)

typedef int (*RCALLBACK)(int fd);
struct conn
{
    int fd;

    char buffer[BUFFER_LENGTH];
    int length;

    RCALLBACK send_callback;

    union
    {
        RCALLBACK recv_callback;
        RCALLBACK accept_callback;
    } r_action;
};

int set_event(int fd, int event, int flag);
int event_register(int fd, int event);
int accept_cb(int fd);
int recv_cb(int fd);
int send_cb(int fd);
int init_server(unsigned short port);
int init_connections();
void cleanup_connections();

#endif
