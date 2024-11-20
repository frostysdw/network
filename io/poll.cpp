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
  /*初始化pollfd数组*/
  struct pollfd fds[1024] = {0};
  fds[sockfd].fd = sockfd;
  fds[sockfd].events = POLLIN; // 等待可读事件
  int maxfd = sockfd;
  while (1) {
    /*调用poll，等待文件描述符的事件*/
    int nready = poll(fds, maxfd + 1, -1); // -1表示无限等待
    /*判断是否可读*/
    if (fds[sockfd].revents & POLLIN) {
      int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &len);
      printf("accept finshed: %d\n", clientfd);
      fds[clientfd].fd = clientfd;
      fds[clientfd].events = POLLIN;
      if (clientfd > maxfd)
        maxfd = clientfd;
    }
    int i = 0;
    /*遍历所有可能的客户端套接字*/
    for (i = sockfd + 1; i <= maxfd; i++) {
      /*检查是否有数据可读*/
      if (fds[i].revents & POLLIN) {
        char buffer[1024] = {0};
        int count = recv(i, buffer, 1024, 0);
        /*判断是否断开连接*/
        if (count == 0) { 
          printf("client disconnect: %d\n", i);
          close(i);
          fds[i].fd = -1;
          fds[i].events = 0;
          continue;
        }
        printf("RECV: %s\n", buffer);
        count = send(i, buffer, count, 0);
        printf("SEND: %d\n", count);
      }
    }
  } 
  /*中断程序*/
  getchar();
  printf("exit\n");
  return 0;
}
