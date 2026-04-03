#include "../../include/core/EventLoop.h"
#include "../../include/core/Epoll.h"
#include "../../include/core/Channel.h"
#include "../../include/core/Connection.h"
#include <sys/syscall.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>

EventLoop::EventLoop(int eptime_out, bool inmainloop, int time_out, int time_interval) : 
    ep_(new Epoll(Epoll::create_epoll())), eptime_out_(eptime_out), stop_(false),                   // 创建 epoll 句柄描述符，并将其赋予循环
    wakeup_fd_(create_wakeupfd()), wakeup_channel_(std::make_unique<Channel>(wakeup_fd_, this)),    // 创建异步唤醒描述符和其事件分发器
    inmainloop_(inmainloop), time_interval_(time_interval), time_out_(time_out),                    // 定时器描述符运行所需参数
    timer_fd_(create_timefd()), timer_channel_(std::make_unique<Channel>(timer_fd_, this))          // 创建定时器描述符和其事件分发器
{ // wakeup_fd_ 和 timer_fd_ 不同于往往伴随了海量数据的常规 sockfd，不使用边缘触发会频繁通知降低 epoll 性能，而二者只是一个不带有数据的计时器，IO / 管理连接 实质在其他函数中完成，所以使用水平触发 
    wakeup_channel_->set_readcallback([this]() { this->handle_wakeup(); });
    wakeup_channel_->enablereading();
    reset_time();
    timer_channel_->set_readcallback([this]() { this->handle_timeout(); });
    timer_channel_->enablereading();
} 

EventLoop::~EventLoop() { }

int EventLoop::get_timeout() const { return eptime_out_; }
bool EventLoop::in_loopthread() { return threadid_ == syscall(SYS_gettid); }

void EventLoop::run() {
    threadid_ = syscall(SYS_gettid);
    while (stop_ == false) {                                            // 循环 epoll_wait() 接收触发的事件
        std::vector<Channel*> rtn_channels = ep_->loop(eptime_out_);    // 获取触发的事件
        if (rtn_channels.empty()) eptimeout_callback_(this);            // 如果没有事件触发，回调处理超时
        else for (auto it : rtn_channels) it->handle();                 // 处理触发的事件
    }
}

void EventLoop::stop() {
    stop_ = true;
    wakeup();       // 唤醒阻塞的 epoll_wait()，使循环立刻识别到标志位的变化，立即停止循环
}

void EventLoop::update_channel(Channel* ch) { ep_->update_channel(ch); }
void EventLoop::remove_channel(Channel* ch) { ep_->remove_channel(ch); }

void EventLoop::set_eptimeoutcallback(std::function<void(EventLoop*)> func) { eptimeout_callback_ = func; }
void EventLoop::set_timeoutcallback(std::function<void(spConnection)> func) { timeout_callback_ = func; }

int EventLoop::create_wakeupfd() {
    int wakeup_fd = eventfd(0, EFD_NONBLOCK);
    if (wakeup_fd == -1) {
        printf("%s:%s:%d wakeup_fd create error:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        exit(-1);
    }
    return wakeup_fd;
}

void EventLoop::addtask(std::function<void()> func) {
    // 锁作用域，防止 comp 线程中调用的 addtask 与 IO 线程中工作的 handle_wakeup() 抢夺 task_queue_ 任务队列资源造成任务遗漏和重做
    {
        std::lock_guard<std::mutex> lock(wakeup_mutex_);
        taskqueue_.push(func);
    }
    // 一个线程中只能调用一个阻塞函数，否则多重阻塞可能导致无法唤醒，循环中已经调用了 epoll_wait，这里无法用条件变量
    // 只能用 epoll 监听一个新的 fd，这个 fd 专门监听 wakeup 的读事件，把事件交给循环监听，由其触发开启任务，是真正将写缓冲区任务交给 IO 线程的步骤
    wakeup(); 
}

void EventLoop::wakeup() {
    uint64_t val = 1;
    write(wakeup_fd_, &val, sizeof(val));   // 向 wakeup_fd_ 中写入任意数据，使其触发读事件，唤醒 handle_wakeup() 处理任务队列
}

void EventLoop::handle_wakeup() {
    uint64_t val;
    read(wakeup_fd_, &val, sizeof(val));    // 读出 wakeup_fd_ 中数据，停止其读事件触发，由于计数数据是一次读完的，类似边缘触发，所以 wakeup_fd_ 不需要重复设置边缘触发
    std::function<void()> task;
    std::queue<std::function<void()>> local_tasks;
    
    // 先将任务队列中的任务转移到本地队列，减少加锁时间
    { 
        std::lock_guard<std::mutex> lock(wakeup_mutex_);    // 任务队列加锁
        local_tasks.swap(taskqueue_);
    }
    
    // 处理本地队列中的任务
    while (!local_tasks.empty()) {
        task = std::move(local_tasks.front());
        local_tasks.pop();
        task();
    }
}

int EventLoop::create_timefd() {
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    if (timer_fd == -1) {
        printf("%s:%s:%d timer_fd create error:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        exit(-1);
    }
    return timer_fd;
} 

void EventLoop::reset_time() {
    struct itimerspec time;
    memset(&time, 0, sizeof(time));
    time.it_value.tv_sec = time_interval_;      // 设置首次检查连接时间间隔
    time.it_value.tv_nsec = 0;
    time.it_interval.tv_sec = time_interval_;   // 设置后续检查连接时间间隔
    time.it_interval.tv_nsec = 0;
    timerfd_settime(timer_fd_, 0, &time, nullptr);
}

void EventLoop::handle_newconn(spConnection conn) { // 主线程
    std::lock_guard<std::mutex> lock(conn_mutex_);
    conns_[conn->get_fd()] = conn;
}

void EventLoop::handle_timeout() {
    uint64_t val;
    read(timer_fd_, &val, sizeof(val)); // 读出定时器到期次数，停止其读事件触发
    if (inmainloop_);
    else {
        time_t now = time(0);
        for (auto it = conns_.begin(); it != conns_.end(); ) {
            if (it->second->time_out(now, time_out_)) {
                timeout_callback_(it->second);
                std::lock_guard<std::mutex> lock(conn_mutex_);
                it = conns_.erase(it);  // erase() 返回值为被删除元素后的下一个迭代器，如果删除则原地遍历下一个
            }
            else ++it;                  // 如果不删除则正常遍历下一个
        }
    }
}
