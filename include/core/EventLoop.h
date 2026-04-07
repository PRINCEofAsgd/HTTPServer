#pragma once
#include <queue>
#include <map>
#include <functional>
#include <memory>
#include <mutex>
#include <atomic>

class Epoll;
class Channel;
class Connection;
using spConnection = std::shared_ptr<Connection>;

class EventLoop {
private:
    std::unique_ptr<Epoll> ep_;                             // 监听所有事件的 epoll 句柄指针
    int eptime_out_;                                        // 循环监听事件的超时时间，单位为毫秒
    std::function<void(EventLoop*)> eptimeout_callback_;    // 处理循环监听超时事件的回调函数声明
    std::atomic_bool stop_;                                 // 控制循环是否进行的标志位

    int threadid_;                                          // 事件循环所在线程的 id
    // 任务队列
    std::queue<std::function<void()>> taskqueue_;           // 业务层传来的发送消息任务队列
    int wakeup_fd_;                                         // 监听发送消息任务队列是否有读事件的 fd 
    std::unique_ptr<Channel> wakeup_channel_;               // 发送消息 fd 的事件分发器
    std::mutex wakeup_mutex_;                               // 保护发送消息任务队列的锁

    bool inmainloop_;                                       // 当前对象是否在主线程中(是否需要处理连接超时)
    int time_interval_;                                     // EventLoop 遍历 conns_ 检查哪些连接超时的时间间隔
    int time_out_;                                          // Connection 未发送消息连接超时的时限
    int timer_fd_;                                          // 监听超时事件的 fd 
    std::unique_ptr<Channel> timer_channel_;                // 监听超时 fd 的事件分发器
    std::map<int, spConnection> conns_;                     // 存放当前线程循环中未超时的对端连接的图
    std::mutex conn_mutex_;                                 // 保护操作对端连接存取的锁
    std::function<void(spConnection)> timeout_callback_;    // 处理对端连接超时的回调函数
    
public:
    EventLoop(const int eptime_out = -1, bool inmainloop = false, const int time_out = 5, const int time_interval = 3); 
    ~EventLoop();

    int get_timeout() const;
    bool in_loopthread();
    
    void run();                         // 循环遍历 epoll 句柄中监听到触发的事件
    void stop();
    
    void update_channel(Channel* ch);   // 将事件 Channel 加入监听句柄 epoll 监听
    void remove_channel(Channel* ch);   // 将事件 Channel 从监听句柄 epoll 中移除
    
    void set_eptimeoutcallback(std::function<void(EventLoop*)> func); // 设置 epoll_wait() 超时回调函数，回调业务层处理
    void set_timeoutcallback(std::function<void(spConnection)> func); // 设置长时间未收到事件超时回调函数
    
    static int create_wakeupfd();
    void addtask(std::function<void()> func);   // 把业务层传来的写任务加入其所属的循环 IO 线程处理
    void wakeup();                              // 设置唤醒本线程 wakeup_fd_ 的标志位
    void handle_wakeup();                       // 唤醒 wakeup_fd_ 处理任务
    
    static int create_timefd();       // 创建、设置定时器描述符时间间隔参数的函数
    void reset_time();
    void handle_newconn(spConnection conn);     // 保存所有本线程的循环负责的 Connection
    void handle_timeout();                      // 每经设定 time_interval_ 的时间间隔遍历检查所有 Connection，将 time_out_ 时间内无读事件的 Connection 删除
};
