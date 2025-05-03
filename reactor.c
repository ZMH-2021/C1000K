

#include "reactor.h"

int epfd = 0;
struct conn *conn_list = NULL;

struct timeval begin;

int set_event(int fd, int event, int flag)
{

    if (flag)
    {

        struct epoll_event ev;
        ev.events = event;
        ev.data.fd = fd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    }
    else
    {

        struct epoll_event ev;
        ev.events = event;
        ev.data.fd = fd;
        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    }
}

int event_register(int fd, int event)
{

    if (fd < 0)
        return -1;

    conn_list[fd].fd = fd;
    conn_list[fd].r_action.recv_callback = recv_cb;
    conn_list[fd].send_callback = send_cb;

    memset(conn_list[fd].buffer, 0, BUFFER_LENGTH);
    conn_list[fd].length = 0;

    set_event(fd, event, 1);
}

int accept_cb(int fd)
{

    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);

    int clientfd = accept(fd, (struct sockaddr *)&clientaddr, &len);

    if (clientfd < 0)
    {
        printf("accept errno: %d --> %s\n", errno, strerror(errno));
        return -1;
    }

    event_register(clientfd, EPOLLIN | EPOLLET); // | EPOLLET

    if ((clientfd % TIME_PRINT) == 0)
    {

        struct timeval current;
        gettimeofday(&current, NULL);

        int time_used = TIME_SUB_MS(current, begin);
        memcpy(&begin, &current, sizeof(struct timeval));

        printf("accept finshed: %d, time_used: %d ms\n", clientfd, time_used);
    }

    return 0;
}

int recv_cb(int fd)
{

    memset(conn_list[fd].buffer, 0, BUFFER_LENGTH);
    int count = recv(fd, conn_list[fd].buffer, BUFFER_LENGTH, 0);
    if (count == 0)
    { // disconnect
        printf("client disconnect: %d\n", fd);
        close(fd);

        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL); // unfinished

        return 0;
    }
    else if (count < 0)
    { //

        printf("count: %d, errno: %d, %s\n", count, errno, strerror(errno));
        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);

        return 0;
    }

    conn_list[fd].length = count;

    set_event(fd, EPOLLOUT, 0);

    return count;
}

int send_cb(int fd)
{

    int count = 0;

    if (conn_list[fd].length != 0)
    {
        count = send(fd, conn_list[fd].buffer, conn_list[fd].length, 0);
    }

    set_event(fd, EPOLLIN, 0);

    return count;
}

int init_server(unsigned short port)
{

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0
    servaddr.sin_port = htons(port);              // 0-1023,

    if (-1 == bind(sockfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)))
    {
        printf("bind failed: %s\n", strerror(errno));
    }

    listen(sockfd, 10);

    return sockfd;
}

int init_connections()
{
    conn_list = calloc(CONNECTION_SIZE, sizeof(struct conn));
    if (!conn_list)
    {
        perror("calloc failed");
        return -1;
    }
    return 0;
}

void cleanup_connections()
{
    if (conn_list)
    {
        for (int i = 0; i < CONNECTION_SIZE; i++)
        {
            if (conn_list[i].fd > 0)
                close(conn_list[i].fd);
        }
        free(conn_list);
        conn_list = NULL;
    }
}

int main()
{

    unsigned short port = 2000;

    if (init_connections() != 0)
    {
        return EXIT_FAILURE;
    }

    epfd = epoll_create(CONNECTION_SIZE);

    int i = 0;

    for (i = 0; i < MAX_PORTS; i++)
    {

        int sockfd = init_server(port + i);
        printf("listen port at %d\n", port + i);

        conn_list[sockfd].fd = sockfd;
        conn_list[sockfd].r_action.recv_callback = accept_cb;

        set_event(sockfd, EPOLLIN, 1);
    }

    gettimeofday(&begin, NULL);

    while (1)
    { // mainloop

        struct epoll_event events[MAX_EVENTS] = {0};
        int nready = epoll_wait(epfd, events, MAX_EVENTS, -1);

        int i = 0;
        for (i = 0; i < nready; i++)
        {

            int connfd = events[i].data.fd;

            if (events[i].events & EPOLLIN)
            {
                conn_list[connfd].r_action.recv_callback(connfd);
            }

            if (events[i].events & EPOLLOUT)
            {
                conn_list[connfd].send_callback(connfd);
            }
        }
    }
    cleanup_connections();
    return 0;
}
