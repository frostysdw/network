#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

/*线程处理*/
void *client_thread(void *arg) {
  int clientfd = *(int *)arg;
  while (1) {
    /*定义缓冲区 buffer，存储接收的数据*/
    char buffer[1024] = {0};
    int count = recv(clientfd, buffer, 1024, 0);
    if (count == 0) { // 客户端断开连接
      printf("client disconnect: %d\n", clientfd);
      close(clientfd);
      break;
    }
    printf("RECV: %s\n", buffer);
    /*发送数据*/
    count = send(clientfd, buffer, count, 0);
    printf("SEND: %d\n", count);
  }
  return 0;
}

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
  printf("accept\n");
  while (1) {
    /*阻塞程序, 等待客户端连接*/
    int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &len);
    printf("accept finshed\n");
    pthread_t thid; //声明线程id
    /*创建线程*/
    pthread_create(&thid, NULL, client_thread, &clientfd); 
  }
  /*中断程序*/
  getchar();
  printf("exit\n");
  return 0;
}
