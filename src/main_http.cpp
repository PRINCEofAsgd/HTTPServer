#include "../include/http/HttpServer.h"
#include "../include/core/TcpServer.h"
#include <iostream>
#include <csignal>

using namespace std;

TcpServer* tcpserver;
HttpServer *httpserver;

void stop(int sig) {
    printf("sig = %d\n", sig);
    httpserver->stop();
    tcpserver->stop();
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 1) {
        printf("Use: ./HttpServerExample\n");
        printf("Example: ./HttpServerExample\n");
        return -1;
    }

    signal(SIGTERM, stop);
    signal(SIGINT, stop);

    string ip = "192.168.1.128";
    string port = "8079";
    int threadnum_IO = 4;
    int threadnum_comp = 5;
    int eptime_out = -1;
    int time_out = 60;
    int time_interval = 10;
    int sep = 2;
    int max_requests = 100; // 每个连接的最大请求数
    
    // 创建TCP服务器，使用HTTP协议（sep=2）
    tcpserver = new TcpServer(ip, port, threadnum_IO, eptime_out, time_out, time_interval, sep, max_requests);
    
    // 创建HTTP服务器
    httpserver = new HttpServer(tcpserver, threadnum_comp);
    
    // 启动服务器
    httpserver->start();
    
    return 0;
}