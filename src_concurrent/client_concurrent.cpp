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
#include <signal.h>
#include "../include/core/Buffer.h"

using namespace std;

int sockfd = -1;
time_t pre = time(0);
int messagenum = 0;

void stop(int sign) {
    close(sockfd);
    printf("exit with sign: %d\n", sign);
    time_t now = time(0);
    printf("间隔时间：%d\n", int(now - pre));
    printf("共接收到 %d 条消息\n", messagenum);
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        printf("usage:./client_concurrent ip port message_count\n"); 
        printf("example:./client_concurrent 192.168.1.101 4819 1000\n"); 
        return -1;
    }

    signal(SIGINT, stop);
    signal(SIGTERM, stop);
    
    struct sockaddr_in servaddr;
 
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        printf("socket() failed.\n"); 
        return -1; 
    }
    
    // 设置套接字为非阻塞模式
    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);
    
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
    Buffer send_buffer(1);  // 使用分隔符类型1（4字节长度头部）
    Buffer recv_buffer(1);  // 使用分隔符类型1（4字节长度头部）
    int circlenum = atoi(argv[3]);
    long send_count = 0;

    // 发送循环
    for (int ii = 0; ii < circlenum; ++ii) {
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "这是第%d个试验数据。", ii);

        // 使用发送缓冲区添加带分隔符的消息
        send_buffer.append_withsep(buf, strlen(buf));
        send_count += strlen(buf);

        // 从发送缓冲区中发送数据
        while (send_buffer.size() > 0) {
            int send_len = send_buffer.putmessage(buf, sizeof(buf));
            int sent = send(sockfd, buf, send_len, 0);
            send_count -= sent;
            if (sent < 0) {
                if (errno == EINTR) continue;
                if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                break;
            }
        }
    }

    // 接收循环
    time_t last_activity_time = time(0);
    const int TIMEOUT_SECONDS = 20;  // 20秒超时
    
    while (messagenum < circlenum) {
        memset(buf, 0, sizeof(buf));
        int red = recv(sockfd, buf, sizeof(buf), 0);

        if (red > 0) {                          // 成功读取数据，加入接收缓冲区
            if (recv_buffer.size() + red > max_buffer_size) {
                break;
            }
            recv_buffer.append(buf, red);
            last_activity_time = time(0);       // 更新最后活动时间
        } 
        else if (red == -1 && errno == EINTR) { // EINTR 表示被信号中断，继可重新读取数据
            continue;
        } 
        else if (red == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {    // EAGAIN 或 EWOULDBLOCK 表示没有数据可读，处理已读取的数据
            bool processed = false;
            while (true) {                      // 循环拆包，处理接收缓冲区中的数据
                std::shared_ptr<std::string> message = std::make_shared<std::string>();
                if (!recv_buffer.getmessage(message)) {
                    break;
                }
                ++messagenum;
                printf("recv: %s\n", message->c_str());
                processed = true;
                last_activity_time = time(0);   // 更新最后活动时间
            }
            
            if (messagenum >= circlenum) {      // 检查是否已经接收到了所有预期的消息
                break;
            }
            
            if (time(0) - last_activity_time > TIMEOUT_SECONDS) {   // 检查是否超时
                printf("接收超时，退出循环\n");
                break;
            }
            
            if (!processed) {                   // 只有在没有处理任何数据时才使用 usleep
                // printf("recv_buffer size: %ld\n", recv_buffer.size());
                usleep(1000);
            }
        } 
        else {                                  // 其他错误
            printf("连接关闭或错误，退出循环\n");
            break;
        }
    }

    time_t now = time(0);
    printf("间隔时间：%d\n", int(now - pre));
    printf("共接收到 %d 条消息\n", messagenum);

    close(sockfd);
    return 0;
} 
