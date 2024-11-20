#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  /*创建TCP套接字
  AF_INET=IPv4地址，SOCK_STREAM=面向连接的套接字，0表示默认的协议（通常是TCP协议）*/
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  /*设置服务器地址*/
  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0 表示任意网络地址
  servaddr.sin_port = htons(2000);              // 端口号
  if (-1 ==
      bind(sockfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr))) {
    printf("bind failed: %s\n", strerror(errno));
  }
  /*监听客户端请求*/
  listen(sockfd, 10);
  printf("listen finshed: %d\n",
         sockfd); // 打印监听的套接字描述符
                  // 通常是3（因为0、1、2已被标准输入、输出、错误使用）
  /*存储客户端的地址信息**/
  struct sockaddr_in clientaddr;
  socklen_t len = sizeof(clientaddr);
  /*创建epoll实例*/
  int epfd = epoll_create(1); // 参数1没有实际意义
  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = sockfd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
  while (1) {
    /*创建数组events存储待发生的事件*/
    struct epoll_event events[1024] = {0};
    int nready = epoll_wait(epfd, events, 1024, -1);
    int i = 0;
    for (i = 0; i < nready; i++) {
      /*获取触发事件的文件描述符*/
      int connfd = events[i].data.fd;
      /*判断是否是监听套接字*/
      if (connfd == sockfd) {
        int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &len);
        printf("accept finshed: %d\n", clientfd);
        ev.events = EPOLLIN;
        ev.data.fd = clientfd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);
      } else if (events[i].events & EPOLLIN) {
        char buffer[1024] = {0};
        int count = recv(connfd, buffer, 1024, 0);
        /*判断是否断开连接*/
        if (count == 0) { 
          printf("client disconnect: %d\n", connfd);
          close(connfd);
          epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);
          continue;
        }
        printf("RECV: %s\n", buffer);
        count = send(connfd, buffer, count, 0);
        printf("SEND: %d\n", count);
      }
    }
  }
  /*中断程序*/
  getchar();
  printf("exit\n");
  return 0;
}

