#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_LENGTH 1024
#define CONNECTION_SIZE 1048576

typedef int(*RCALLBACK)(int fd);

int accept_cb(int fd);
int recv_cb(int fd);
int send_cb(int fd);
int epfd = 0;

struct conn{
    int fd;
    char rbuffer[BUFFER_LENGTH];
    int rlength;
    char wbuffer[BUFFER_LENGTH];
    int wlength;
    RCALLBACK send_callback;
    union {
        RCALLBACK recv_callback;
        RCALLBACK accept_callback;
    }r_action;
};

struct conn conn_list[CONNECTION_SIZE] = {0};

void set_event(int fd, int event, int flag){
    if (flag) {
        struct epoll_event ev;
        ev.events = event;
        ev.data.fd = fd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    }else {
        struct epoll_event ev;
        ev.events = event;
        ev.data.fd = fd;
        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    }
}

void event_register(int fd, int event){
    conn_list[fd].fd = fd;
    conn_list[fd].r_action.recv_callback = recv_cb;
    conn_list[fd].send_callback = send_cb;
    memset(conn_list[fd].rbuffer, 0, BUFFER_LENGTH);
    conn_list[fd].rlength = 0;
    memset(conn_list[fd].wbuffer, 0, BUFFER_LENGTH);
    conn_list[fd].wlength = 0;
    set_event(fd, event, 1);  
}

int accept_cb(int fd){
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    int clientfd = accept(fd, (struct sockaddr *)&clientaddr, &len);
    printf("accept finshed: %d\n", clientfd);
    event_register(clientfd, EPOLLIN);
    return clientfd;
}

int recv_cb(int fd){
    memset(conn_list[fd].rbuffer, 0, BUFFER_LENGTH);
    int count = recv(fd, conn_list[fd].rbuffer, BUFFER_LENGTH, 0);
    if (count == 0) { 
        printf("client disconnect: %d\n", fd);
        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        return 0;
    }
    conn_list[fd].rlength = count;
    printf("RECV: %s\n", conn_list[fd].rbuffer);
    conn_list[fd].wlength = conn_list[fd].rlength;
    memcpy(conn_list[fd].wbuffer, conn_list[fd].rbuffer, conn_list[fd].wlength);
    set_event(fd, EPOLLOUT, 0);
    return count;
}

int send_cb(int fd){
    int count = send(fd, conn_list[fd].wbuffer, conn_list[fd].wlength, 0);
    set_event(fd, EPOLLIN, 0);
    return count;
}

int init_server(unsigned short port){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(port);
    if (-1 == bind(sockfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr))) {
        printf("bind failed: %s\n", strerror(errno));
        }
    listen(sockfd, 10);
    printf("listen finshed: %d\n", sockfd); 
    return sockfd;
}

int main(){
    unsigned short port = 2000;
    int sockfd = init_server(port); //初始化服务器套接字
    epfd = epoll_create(1); //创建epoll 实例
    conn_list[sockfd].fd = sockfd;
    conn_list[sockfd].r_action.accept_callback = accept_cb;
    set_event(sockfd, EPOLLIN, 1); //监听事件
    while(1){
        struct epoll_event events[1024] = {0};
        int nready = epoll_wait(epfd, events, 1024, -1);
        int i = 0;
        for(i = 0; i < nready; i++){
            int connfd = events[i].data.fd;
            if(events[i].events & EPOLLIN){
                conn_list[connfd].r_action.accept_callback(connfd);
            }
            if(events[i].events & EPOLLOUT){
                conn_list[connfd].send_callback(connfd);
            }
        }
    }
    return 0;
}