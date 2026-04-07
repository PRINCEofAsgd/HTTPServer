// 网络通讯的客户端程序(并发通讯)。
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <thread>
#include <chrono>
// #include "../include/Buffer.h"

using namespace std;

int main(int argc, char *argv[])
{
    if (argc != 4) {
        printf("usage:./client_concurrent ip port message_count\n"); 
        printf("example:./client_concurrent 192.168.1.101 4819 1000\n"); 
        return -1;
    }

    int sockfd;
    struct sockaddr_in servaddr;
 
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        printf("socket() failed.\n"); 
        return -1; 
    }
    
    // 设置套接字为非阻塞模式
    // fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);
    
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    // 非阻塞模式下的连接
    int ret = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (ret < 0 && errno != EINPROGRESS) {
        printf("connect(%s:%s) failed.\n",argv[1],argv[2]); 
        close(sockfd); 
        return -1;
    }

    char buf[1024];
    size_t max_buffer_size = 64 * 1024 * 1024;
    printf("connect ok.\n");
    
    // 创建发送缓冲区和接收缓冲区
    // Buffer send_buffer(1);  // 使用分隔符类型1（4字节长度头部）
    // Buffer recv_buffer(1);  // 使用分隔符类型1（4字节长度头部）
    int circlenum = atoi(argv[3]);
    int messagenum = 0;
    time_t pre = time(0);

    // 发送循环
    for (int ii = 0; ii < circlenum; ++ii) {
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "这是第%d个试验数据。", ii);

        uint32_t hostlen = strlen(buf);
        uint32_t netlen = htonl(hostlen);
        char tempbuf[1024];
        memset(tempbuf, 0, sizeof(tempbuf));
        memcpy(tempbuf, &netlen, 4);
        memcpy(tempbuf + 4, buf, hostlen);
        
        if (send(sockfd, tempbuf, hostlen + 4, 0) <= 0) { // 把命令行输入的内容发送给服务端
            printf("write() failed.\n"); 
            close(sockfd); 
            return -1; 
        }

        memset(buf, 0, sizeof(buf));
        memset(&netlen, 0, sizeof(netlen));
        if (recv(sockfd, &netlen, 4, 0) < 4) {      // 先读取4字节的报文头部
            printf("read() len failed.\n"); 
            close(sockfd); 
            return -1;
        }

        hostlen = ntohl(netlen);                    // 将网络字节序转换为主机字节序，得到要接收的报文内容的长度
        memset(buf, 0, sizeof(buf));
        if (recv(sockfd, buf, hostlen, 0) < hostlen) {  // 接收服务端的回应
            printf("read() buffer failed.\n"); 
            close(sockfd); 
            return -1;
        }
        printf("recv:%s\n", buf);

        ++messagenum;

    }

    time_t now = time(0);
    printf("间隔时间：%d\n", int(now - pre));
    printf("共接收到 %d 条消息\n", messagenum);

    close(sockfd);
    return 0;
} 
