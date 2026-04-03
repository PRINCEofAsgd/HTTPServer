#include "../../include/client/ClientLoop.h"
#include "../../include/client/ClientConn.h"
#include "../../include/client/Utils.h"
#include "../../include/http/HttpRequest.h"
#include <sys/syscall.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <unistd.h>
#include <iostream>
#include <ctime>
#include <cstring>

ClientLoop::ClientLoop(int time_interval) : client_conn_(nullptr), stop_(false), 
wakeup_fd_(create_wakeupfd()), timer_fd_(create_timerfd()), time_interval_(time_interval) {
    // 添加wakeup_fd_
    struct pollfd pfd_wakeup;
    pfd_wakeup.fd = wakeup_fd_;
    pfd_wakeup.events = POLLIN;
    pfd_wakeup.revents = 0;
    pfds_.push_back(pfd_wakeup);
    
    // 添加timer_fd_
    struct pollfd pfd_timer;
    pfd_timer.fd = timer_fd_;
    pfd_timer.events = POLLIN;
    pfd_timer.revents = 0;
    pfds_.push_back(pfd_timer);
    
    reset_timer();
}

ClientLoop::~ClientLoop() {
    stop();
    if (wakeup_fd_ >= 0) close(wakeup_fd_);
    if (timer_fd_ >= 0) close(timer_fd_);
}

int ClientLoop::create_wakeupfd() {
    int wakeup_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wakeup_fd == -1) {
        printf("%s:%s:%d wakeup_fd create error:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        exit(-1);
    }
    return wakeup_fd;
}

int ClientLoop::create_timerfd() {
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    if (timer_fd == -1) {
        printf("%s:%s:%d timer_fd create error:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        exit(-1);
    }
    return timer_fd;
}

void ClientLoop::run() {
    while (stop_ == false) { // 循环 poll() 接收触发的事件
        updatePollFds();
        int ret = poll(pfds_.data(), pfds_.size(), -1); // 获取触发的事件
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }
        
        if (ret == 0) {}

        else for (size_t i = 0; i < pfds_.size(); i++) { // 处理触发的事件
            if (pfds_[i].revents & POLLIN) { // 可读事件
                if (pfds_[i].fd == wakeup_fd_) handle_wakeup();         // 处理唤醒事件
                else if (pfds_[i].fd == timer_fd_) handle_heartbeat();  // 处理心跳事件
                else handleEvent();                                     // 处理业务事件
            }
            if (pfds_[i].revents & POLLOUT) if (client_conn_) client_conn_->processTask(); // 处理可写事件
        }
    }
}

void ClientLoop::stop() {
    stop_ = true;
    wakeup(); // 唤醒阻塞的 poll()，使循环立刻识别到标志位的变化，立即停止循环
}

// 将客户端连接保存到 client_conn_ 中(可扩展多连接)
void ClientLoop::handle_newconn(spClientConn conn) {
    client_conn_ = conn;
    updatePollFds();
}

// 更新 sockfd 文件描述符：更新写事件或更新有效 fd
void ClientLoop::updatePollFds() {
    // 检查是否需要添加或更新sockfd

    // 如果有客户端连接
    if (client_conn_) { 
        int sockfd = client_conn_->get_fd();

        // 如果 sockfd 有效
        if (sockfd > 0) { 
            bool exists = false; // 检查是否已经存在该sockfd

            for (size_t i = 2; i < pfds_.size(); i++) { // 前两个是 wakeup_fd_ 和 timer_fd_
                // 如果存在该 sockfd ，更新事件类型
                if (pfds_[i].fd == sockfd) { 
                    pfds_[i].events = POLLIN;
                    if (client_conn_->hasTask()) pfds_[i].events |= POLLOUT;
                    pfds_[i].revents = 0;
                    exists = true;
                    break;
                }
            }
            
            // 如果不存在，添加新的 sockfd 文件描述符到 poll() 队列
            if (!exists) {
                struct pollfd pfd_socket;
                pfd_socket.fd = sockfd;
                pfd_socket.events = POLLIN;
                if (client_conn_->hasTask()) pfd_socket.events |= POLLOUT;
                pfd_socket.revents = 0;
                pfds_.push_back(pfd_socket);
            }
        } 

        // 如果 sockfd 无效，移除无效的sockfd
        else for (size_t i = 2; i < pfds_.size();) { 
            if (pfds_[i].fd != wakeup_fd_ && pfds_[i].fd != timer_fd_) 
                pfds_.erase(pfds_.begin() + i);
            else i++;
        }
    } 

    // 如果没有客户端连接，移除所有socket文件描述符，保留wakeup_fd_和timer_fd_
    else while (pfds_.size() > 2) pfds_.pop_back(); 
}

// 处理业务事件
void ClientLoop::handleEvent() { if (client_conn_) client_conn_->recv_message(); }

void ClientLoop::addtask(std::function<void()> func) {
    // 锁作用域，防止多线程抢夺任务队列资源
    {
        std::lock_guard<std::mutex> lock(wakeup_mutex_);
        taskqueue_.push(func);
    }
    wakeup(); 
}

void ClientLoop::wakeup() {
    uint64_t val = 1;
    write(wakeup_fd_, &val, sizeof(val));   // 向 wakeup_fd_ 中写入任意数据，使其触发读事件
}

void ClientLoop::handle_wakeup() {
    uint64_t val;
    read(wakeup_fd_, &val, sizeof(val)); // 读出 wakeup_fd_ 中数据，停止其读事件触发
    std::function<void()> task;
    std::queue<std::function<void()>> local_tasks;
    
    // 先将任务队列中的任务转移到本地队列，减少加锁时间
    { 
        std::lock_guard<std::mutex> lock(wakeup_mutex_); // 任务队列加锁
        local_tasks.swap(taskqueue_);
    }
    
    // 处理本地队列中的任务
    while (!local_tasks.empty()) {
        task = std::move(local_tasks.front());
        local_tasks.pop();
        task();
    }
}

void ClientLoop::reset_timer() {
    struct itimerspec time;
    memset(&time, 0, sizeof(time));
    time.it_value.tv_sec = time_interval_;      // 设置首次检查连接时间间隔
    time.it_value.tv_nsec = 0;
    time.it_interval.tv_sec = time_interval_;   // 设置后续检查连接时间间隔
    time.it_interval.tv_nsec = 0;
    timerfd_settime(timer_fd_, 0, &time, nullptr);
}

void ClientLoop::handle_heartbeat() {
    uint64_t val;
    read(timer_fd_, &val, sizeof(val)); // 读出定时器到期次数，停止其读事件触发
    std::cout << "[心跳检测] 触发心跳检测..." << std::endl;
    
    // 如果有客户端连接
    if (client_conn_) { 
        // 构造心跳请求为标准HTTP请求
        HttpRequest request;
        // 自增请求ID
        requestId_++;
        request.setMethod("GET");
        request.setPath("/heartbeat");
        request.setVersion("HTTP/1.1");
        request.addHeader("Host", SERVER_IP + ":" + SERVER_PORT);
        request.addHeader("Connection", "keep-alive");
        request.addHeader("X-Request-ID", std::to_string(requestId_));
        register_callback_(requestId_, ReqType::HEARTBEAT);
        
        // 通过统一的任务队列发送心跳请求
        client_conn_->send_request(request);
    } 

    // 如果没有客户端连接
    else std::cout << "[心跳检测] 客户端连接为空，跳过本次心跳检测" << std::endl;
}

void ClientLoop::set_register_callback(std::function<void(int, ReqType)> callback) { register_callback_ = callback; }