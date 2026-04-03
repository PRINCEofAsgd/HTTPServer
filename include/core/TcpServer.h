#pragma once
#include "Acceptor.h"
#include "Connection.h"
#include "ThreadPool.h"
#include <map>

class Connection;

class TcpServer {
private:
    std::unique_ptr<EventLoop> mainloop_;               // 循环监听事件的主体
    std::vector<std::unique_ptr<EventLoop>> subloops_;  // 子循环监听事件的主体
    Acceptor acc_;                                      // 接收对端连接处理器对象指针
    std::map<int, spConnection> conns_;                 // 存放对端通讯处理器对象指针的图
    int sep_;                                           // 解析报文分隔符的格式

    int threadnum_;                                     // 线程池中线程的数量，用于对一般情况顺序增长的套接字描述符取模，把若干 Connection 公平地分给所有线程
    ThreadPool threadpool_;                             // 线程池，用于并发处理连接事件

    int time_out_;                                      // 连接超时时间（秒）
    int max_requests_;                                  // 每个连接的最大请求数

    std::mutex conn_mutex;
    std::function<void(spConnection)> newconn_callback_;
    std::function<void(spConnection)> closeconn_callback_;
    std::function<void(spConnection)> errorconn_callback_;
    std::function<void(spConnection)> removeconn_callback_;
    std::function<void(spConnection, std::shared_ptr<std::string>)> recv_callback_;
    std::function<void(spConnection)> send_callback_;
    std::function<void(EventLoop*)> eptimeout_callback_;

public:
    TcpServer(const std::string &ip, const std::string &port, const int threadnum_IO = 5, // 使用提供的地址创建 Acceptor 对象
        const int eptime_out = -1, const int time_out = 60, const int time_interval = 3, const int sep = 0, const int max_requests = 100); 
    ~TcpServer();

    void start();   // 开启循环监听
    void stop();

    // 处理不同对端连接事件的函数，本质是管理 Connection 对象的函数，故由下层监听触发，回调本层函数处理，实现解耦
    void handle_newconn(std::unique_ptr<Socket> cltsock);   // 主线程
    void handle_closeconn(spConnection conn);               // 主线程
    void handle_errorconn(spConnection conn);               // 主线程
    void handle_removeconn(spConnection conn);              // IO 线程
    void handle_recvmessage(spConnection conn, std::shared_ptr<std::string> message);
    void handle_sendmessage(spConnection conn);
    void handle_eptimeout(EventLoop* loop);

    // 设置回调函数，回调业务层处理
    void set_newconncallback(std::function<void(spConnection)> func);
    void set_closeconncallback(std::function<void(spConnection)> func);
    void set_errorconncallback(std::function<void(spConnection)> func);
    void set_removeconncallback(std::function<void(spConnection)> func);
    void set_recvcallback(std::function<void(spConnection, std::shared_ptr<std::string>)> func);
    void set_sendcallback(std::function<void(spConnection)> func);
    void set_eptimeoutcallback(std::function<void(EventLoop*)> func);
};
