#pragma once
#include "Channel.h"
#include "Buffer.h"
#include "Timestamp.h"
#include "SendTask.h"
#include <atomic>
#include <sys/sendfile.h>

class Connection;                                   // 前向声明，避免(智能指针类型声明调用)头文件循环依赖
using spConnection = std::shared_ptr<Connection>;   // 声明类的智能指针类型
class EventLoop;
class Socket;
class Channel;

class Connection : public std::enable_shared_from_this<Connection> {    // 继承 enable_shared_from_this，使 Connection 类对象能够创建指向自己的智能指针
private:
    EventLoop* loop_;                       // TcpServer 的主体循环，Connection 需加入此循环监听，不拥有循环对象，只引用它循环监听自己
    std::unique_ptr<Socket> cltsock_;       // 接收对端连接的 listensocket
    std::unique_ptr<Channel> conn_channel_; // 接收对端连接的事件处理器

    Buffer inputbuffer_;                    // 输入缓冲区
    Buffer outputbuffer_;                   // 输出缓冲区
    size_t max_buffer_size_;                // 输入输出缓冲区的最大容量，防止海量数据堆积导致内存占满
    std::atomic_bool disconnect_;           // 对端连接是否已断开的标志位，断开则设置为true，防止其他线程继续处理断开连接的通讯事件
    uint16_t sep_;                          // 分隔符类型标识

    Timestamp last_activetime;
    bool keep_alive_;                       // 是否保持连接
    int max_requests_;                      // 最大请求数
    int request_count_;                     // 当前请求数

    std::function<void(spConnection, std::shared_ptr<std::string>)> recv_callback_;
    std::function<void(spConnection)> send_callback_;
    std::function<void(spConnection)> closeconn_callback_;
    std::function<void(spConnection)> errorconn_callback_;
    
    std::queue<std::shared_ptr<SendTask>> send_tasks_; // 发送任务队列，只有发送状态完成才会出队，保证任务按顺序处理
    std::shared_ptr<SendTask> current_task_; // 当前正在处理的任务

public:
    Connection(EventLoop* loop, std::unique_ptr<Socket> cltsock, uint16_t sep = 0, size_t max_buffer_size = 4 * 1024 * 1024, int max_requests = 100);   // 使用接收到的套接字创建处理器，并加入循环进行监听
    ~Connection();

    // 返回 cltsock_ 的成员。虽然这里不再写类自己的返回方法而是调用 cltsock_ 的接口，可以降低耦合，但是在高并发场景中，解引用耗时严重，故牺牲轻度耦合来换取时间。
    int get_fd() const;
    std::string get_ip() const;
    uint16_t get_port() const;
    bool time_out(time_t now, int time_out);

    // 本层的回调函数，可被下层函数分化回调，也可回调上层函数实现解耦，这些函数都属于底层处理信息的范畴
    void recv_message();    // 处理与对端通讯的函数，有数据可读时，由 channel 监听到后回调
    void send_task();       // 处理发送任务队列中的任务
    void close_conn();      // 处理对端连接关闭
    void error_conn();      // 处理对端连接错误
    
    // 把业务层 comp 线程传来的任务送至 IO 线程
    void addtask_toIOthread(std::shared_ptr<SendTask> task);        // 本函数由业务层 comp 线程执行，将发送任务交给 IO 线程执行
    void addtask_inIOthread(std::shared_ptr<SendTask> task);        // 在 IO 线程中处理发送任务
    
    // 设置回调函数
    void set_recvcallback(std::function<void(spConnection, std::shared_ptr<std::string>)> func);
    void set_sendcallback(std::function<void(spConnection)> func);
    void set_closeconncallback(std::function<void(spConnection)> func);
    void set_errorconncallback(std::function<void(spConnection)> func);
    
    // Keep-Alive 相关方法
    void set_keep_alive(bool keep_alive);
    bool is_keep_alive() const;
    void increment_request_count();
    bool should_close() const;
};
