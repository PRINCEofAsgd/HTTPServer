// 客户端连接类实现
#include "../../include/client/ClientConn.h"
#include "../../include/client/Utils.h"
#include "../../include/core/SendTask.h"
#include "../../include/client/ClientLoop.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

ClientConn::ClientConn(ClientLoop* loop) : 
sockfd_(-1), loop_(loop), buffer_(2),
current_task_(nullptr), last_activetime_(time(nullptr)) 
{
    rebuildConnection();
}
ClientConn::~ClientConn() { if (sockfd_ > 0) close(sockfd_); }

int ClientConn::get_fd() const { return sockfd_; }
bool ClientConn::time_out(time_t now, int time_out) { return now - last_activetime_ > time_out; }
bool ClientConn::hasTask() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return !send_tasks_.empty() || (current_task_ != nullptr);
}
void ClientConn::set_on_response(std::function<void(const std::string&)> on_response) { on_response_ = on_response; }

// 连接已失效，重建连接的方法(或初始化连接)
void ClientConn::rebuildConnection() {
    // 如果套接字描述符大于 0，关闭旧连接
    if (sockfd_ > 0) {
        close(sockfd_);
        sockfd_ = -1;
        std::cout << "[连接管理] 已关闭旧连接" << std::endl;
    }
    
    // 创建新连接
    struct sockaddr_in servaddr;
    int new_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // 如果创建套接字失败，返回
    if (new_sockfd < 0) {
        std::cerr << "[连接管理] 创建套接字失败: " << strerror(errno) << std::endl;
        return;
    }
    
    // 连接服务器
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(SERVER_PORT.c_str()));
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP.c_str());
    // 如果连接服务器失败，返回
    if (connect(new_sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        std::cerr << "[连接管理] 连接服务器失败: " << strerror(errno) << std::endl;
        close(new_sockfd);
        return;
    }
    
    // 更新套接字和最后活跃时间
    sockfd_ = new_sockfd;
    last_activetime_ = time(nullptr);
    std::cout << "[连接管理] 连接创建成功" << std::endl;
    
    // 通知事件循环更新文件描述符
    if (loop_) loop_->addtask([this]() { this->loop_->updatePollFds(); });
}

// 触发读事件，处理业务事件(接收响应并路由至 Handler)
void ClientConn::recv_message() {
    // 检查是否有可读事件
    if (sockfd_ > 0) {
        char buf[4096] = {0};
        int recvlen = recv(sockfd_, buf, sizeof(buf) - 1, 0);
        if (recvlen <= 0) { // 接收失败
            printf("错误: recv() failed.\n");
            std::cout << "[连接管理] 业务请求失败，正在重建连接..." << std::endl;
            rebuildConnection();
        } 
        else { // 将接收到的数据添加到缓冲区
            buffer_.append(buf, recvlen);
            last_activetime_ = time(nullptr);
            
            // 循环从缓冲区中拆解单个消息
            while (true) {
                std::shared_ptr<std::string> message = std::make_shared<std::string>();
                // 尝试从缓冲区中获取单个消息
                if (!buffer_.getmessage(message)) break; // 没有完整的消息，退出循环
                // 直接回调 HandlerFactory 处理响应
                if (on_response_) on_response_(*message);
            }
        }
    }
}

// 发送 HTTP 请求
void ClientConn::send_request(const HttpRequest& request) {
    std::string request_str = request.toString();
    printf("\n构造的HTTP请求:\n");
    printf("%s\n", request_str.c_str());
    
    // 创建 Tasktype 为 BUFFER 的 SendTask 请求信息实例
    auto buffer_task = std::make_shared<SendTask>(TaskType::BUFFER);
    buffer_task->buffer_data = std::make_shared<std::string>(request_str);
    
    {   
        std::lock_guard<std::mutex> lock(queue_mutex_);
        send_tasks_.push(buffer_task);
    }
    
    // 如果请求体有文件内容，使用文件参数创建 Tasktype 为 FILE 的 SendTask 请求体实例
    if (request.useSendfile()) {
        auto file_task = std::make_shared<SendTask>(TaskType::FILE);
        file_task->file_fd = request.getFileFd();
        file_task->file_offset = request.getFileOffset();
        file_task->file_size = request.getFileSize();
        
        {   
            std::lock_guard<std::mutex> lock(queue_mutex_);
            send_tasks_.push(file_task);
        }
    }
    
    // 通知事件循环监听可写事件，回调 processTask() 处理任务
    loop_->addtask([this]() { this->processTask(); }); 
}

// 处理单个写事件发送任务
void ClientConn::processTask() {
    while (true) {
        // 如果当前任务已处理完，从任务队列中获取一个任务
        if (!current_task_) { 
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (send_tasks_.empty()) return; // 任务队列为空，任务处理完，则返回
            current_task_ = send_tasks_.front();
            send_tasks_.pop();
            current_task_->status = TaskStatus::PROCESSING;
        }
        
        // 处理当前任务
        // 处理缓冲区类型任务
        if (current_task_->type == TaskType::BUFFER) { 
            if (current_task_->buffer_data) {
                const std::string& data = *current_task_->buffer_data;
                size_t sent = 0;
                while (sent < data.size()) {
                    ssize_t ret = send(sockfd_, data.data() + sent, data.size() - sent, 0);
                    if (ret < 0) {
                        // 暂时无法发送，等待下一次可写事件
                        if (errno == EAGAIN || errno == EWOULDBLOCK) return; 
                        // 发送错误
                        else { 
                            current_task_->status = TaskStatus::FAILED;
                            current_task_ = nullptr;
                            std::cout << "[连接管理] 业务请求失败，正在重建连接..." << std::endl;
                            rebuildConnection();
                            return;
                        }
                    }
                    sent += ret;
                }
                current_task_->status = TaskStatus::COMPLETED;
                current_task_ = nullptr;
            }
        } 

        // 处理文件类型任务
        else if (current_task_->type == TaskType::FILE) { 
            while (current_task_->file_size > 0) {
                ssize_t sent = sendfile(sockfd_, current_task_->file_fd, &current_task_->file_offset, current_task_->file_size);
                if (sent < 0) {
                    // 暂时无法发送，等待下一次可写事件
                    if (errno == EAGAIN || errno == EWOULDBLOCK) return; 
                    // 发送错误
                    else { 
                        close(current_task_->file_fd);
                        current_task_->status = TaskStatus::FAILED;
                        current_task_ = nullptr;
                        std::cout << "[连接管理] 业务请求失败，正在重建连接..." << std::endl;
                        rebuildConnection();
                        return;
                    }
                }
                current_task_->file_size -= sent;
            }
            close(current_task_->file_fd);
            current_task_->status = TaskStatus::COMPLETED;
            current_task_ = nullptr;
        }
    }
}
