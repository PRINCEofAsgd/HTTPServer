#include <iostream>
#include <csignal>
#include "../include/core/EchoServer.h"
#include "../include/core/TcpServer.h"

using namespace std;

TcpServer* tcpserver;
EchoServer *echoserver;

void stop(int sig) {
    printf("sig = %d\n", sig);
    echoserver->stop();
    tcpserver->stop();
    exit(0);
}

int main(int argc, char* argv[]) 
{
    if (argc != 3) {
        printf("Use: ./httpserver ip port\n");
        printf("Example: ./httpserver 192.168.1.128 4819\n");
        return -1;
    }

    // 传入参数设置
    int threadnum_IO = 4;   // 处理对端通信的 IO 线程数
    int threadnum_comp = 2; // 处理计算业务的 comp 线程数
    int eptime_out = -1;    // epoll_wait() 等待的超时时间(一般不启用回调)
    int time_out = 60;      // 对端连接判定为 active 的时限
    int time_interval = 10; // 检查对端连接是否 active 的时间间隔
    int sep = 1;            // 解读对端报文的间隔符形式

    signal(SIGTERM, stop);
    signal(SIGINT, stop);

    tcpserver = new TcpServer(argv[1], argv[2], threadnum_IO, eptime_out, time_out, time_interval, sep);
    echoserver = new EchoServer(tcpserver, threadnum_comp);
    echoserver->start();

    return 0;

}
