// 客户端连接类
#pragma once

#include "../../include/http/HttpRequest.h"
#include "../../include/core/Buffer.h"
#include "../../include/core/SendTask.h"
#include <string>
#include <atomic>
#include <mutex>
#include <queue>
#include <memory>

class ClientLoop;

// 客户端连接类，负责处理HTTP请求发送和响应接收
class ClientConn : public std::enable_shared_from_this<ClientConn> {
private:
    int sockfd_;        // 套接字文件描述符
    ClientLoop* loop_;  // 事件循环指针
    Buffer buffer_;     // 用于存储响应数据的缓冲区
    
    // 任务队列相关
    std::queue<std::shared_ptr<SendTask>> send_tasks_;  // 发送任务队列
    std::shared_ptr<SendTask> current_task_;            // 当前处理的任务
    std::mutex queue_mutex_;                            // 任务队列锁
    
    // 心跳检测相关
    time_t last_activetime_;    // 最后活跃时间
    void rebuildConnection();   // 连接已失效，重建连接的方法(或初始化连接)

    std::function<void(const std::string&)> on_response_; // 响应处理回调函数
    
public:
    ClientConn(ClientLoop* loop);
    ~ClientConn();
    
    int get_fd() const;                         // 获取文件描述符
    bool hasTask();                             // 检查是否有任务
    bool time_out(time_t now, int time_out);    // 检查是否超时
    void set_on_response(std::function<void(const std::string&)> on_response); // 设置响应处理回调函数

    // 触发读事件，处理业务事件(接收响应并路由至 Handler)
    void recv_message(); 
    // 发送 HTTP 请求(写事件添加到任务队列)
    void send_request(const HttpRequest& request);  
    // 处理单个任务
    void processTask(); 
};