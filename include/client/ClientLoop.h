#pragma once
#include "HandlerRouter.h"
#include <queue>
#include <map>
#include <functional>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>

class ClientConn;
using spClientConn = std::shared_ptr<ClientConn>;

class ClientLoop {
private:
    std::vector<struct pollfd> pfds_;               // poll文件描述符集合
    spClientConn client_conn_;                      // 客户端连接
    std::atomic_bool stop_;                         // 控制循环是否进行的标志位

    // 发送任务队列相关
    int wakeup_fd_;                                 // 监听发送消息任务队列是否有读事件的 fd 
    std::queue<std::function<void()>> taskqueue_;   // 业务层传来的任务队列
    std::mutex wakeup_mutex_;                       // 保护发送消息任务队列的锁

    // 心跳检测相关
    int timer_fd_;                                  // 心跳检测定时器文件描述符
    int time_interval_;                             // 心跳检测时间间隔
    std::function<void(int, ReqType)> register_callback_;        // 心跳检测函数

public:
    ClientLoop(const int time_interval = 20); 
    ~ClientLoop();
    static int create_wakeupfd(); // 创建事件唤醒文件描述符
    static int create_timerfd();  // 创建定时器文件描述符

    void handle_newconn(spClientConn conn);  // 将客户端连接保存到 client_conn_ 中(可扩展多连接)
    void updatePollFds();                       // 更新 sockfd 文件描述符：更新写事件或更新有效 fd
    
    void run(); 
    void stop();

    void handleEvent();                         // 处理业务事件
    
    void addtask(std::function<void()> func);   // 把业务层传来的任务加入 IO 循环处理
    void wakeup();                              // 设置唤醒本线程 wakeup_fd_ 的标志位
    void handle_wakeup();                       // 唤醒 wakeup_fd_ 处理任务

    void reset_timer();                         // 重置定时器
    void handle_heartbeat();                    // 处理心跳检测事件
    void set_register_callback(std::function<void(int, ReqType)> callback); // 设置注册回调函数
};