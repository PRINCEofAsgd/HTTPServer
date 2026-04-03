#include "../../include/core/Connection.h"
#include <sys/syscall.h>
#include <string.h>

Connection::Connection(EventLoop* loop, std::unique_ptr<Socket> cltsock, uint16_t sep, size_t max_buffer_size, int max_requests) : 
loop_(loop), cltsock_(std::move(cltsock)), inputbuffer_(sep), outputbuffer_(sep), 
max_buffer_size_(max_buffer_size), disconnect_(false), sep_(sep), 
keep_alive_(true), max_requests_(max_requests), request_count_(0), current_task_(nullptr) {
    conn_channel_ = std::make_unique<Channel>(cltsock_->get_fd(), loop_); // 使用已获取的对端连接的 socket 创建处理对端通讯的事件处理器
    // 设置处理器触发后回调函数回调本层 Connection 对象的函数处理不同事件
    conn_channel_->set_readcallback(std::bind(&Connection::recv_message, this));
    conn_channel_->set_writecallback(std::bind(&Connection::send_task, this));
    conn_channel_->set_closecallback(std::bind(&Connection::close_conn, this));
    conn_channel_->set_errorcallback(std::bind(&Connection::error_conn, this));
    conn_channel_->set_evtype_et();     // 设置处理器为边缘触发
    conn_channel_->enablereading();     // 为处理器注册读事件监听新连接
}

Connection::~Connection() { }

int Connection::get_fd() const { return cltsock_->get_fd(); }
std::string Connection::get_ip() const { return cltsock_->get_ip(); }
uint16_t Connection::get_port() const { return cltsock_->get_port(); }
bool Connection::time_out(time_t now, int time_out) { return now - last_activetime.get_toint() > time_out; }

void Connection::close_conn() { 
    disconnect_ = true;
    conn_channel_->remove();
    closeconn_callback_(shared_from_this()); 
}
void Connection::error_conn() { 
    disconnect_ = true;
    conn_channel_->remove();
    errorconn_callback_(shared_from_this()); 
}

void Connection::recv_message() {
    char buffer[1024]; 
    while (true) {  // 边缘触发下，循环读至读完所有信息
        memset(buffer, 0, sizeof(buffer));
        int red = recv(cltsock_->get_fd(), buffer, sizeof(buffer), 0);

        // 返回有效字节数，成功读取数据
        if (red > 0) {                                          // 此处应该做拥塞控制的预防，将来实现
            if (inputbuffer_.size() + red > max_buffer_size_) { // 读入数据后缓冲区超过最大值，为保护内存占满，直接断开连接，杜绝消息堆积和恶意长报文
                error_conn();
                break;
            }
            inputbuffer_.append(buffer, red);                   // 把读取的数据追加到接收缓冲区中
            last_activetime.set_now();
        }

        // 数据读取被信号中断(可重试)
        else if (red == -1 && errno == EINTR) continue; 

        // 非阻塞模式下无数据(数据读完)
        else if (red == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) ) { 
            while (true) {                                      // 循环从缓冲区中拆解单个消息
                // 由于接收到信息需要传递给业务处理，可能涉及多个线程，为避免某个线程提前释放，创建智能指针存储单个消息回调处理
                std::shared_ptr<std::string> message = std::make_shared<std::string>(); 
                if (!inputbuffer_.getmessage(message)) {
                    // std::cout << "error\n" << std::endl;
                    break;   // 尝试从缓冲区中获取单个消息，存储在 message 中，返回是否成功获取到完整消息
                }
                recv_callback_(shared_from_this(), message);    // 成功从缓冲区中获取单个消息，回调上层处理
            }
            break;
        }

        // 对端连接关闭
        else if (red == 0) { 
            close_conn();
            break;
        }
    }
}

void Connection::send_task() {
    // 如果当前没有任务，从任务队列中获取
    if (!current_task_) {
        if (send_tasks_.empty()) {
            conn_channel_->disablewriting();
            send_callback_(shared_from_this());
            return;
        }
        current_task_ = send_tasks_.front();
        send_tasks_.pop();
        current_task_->status = TaskStatus::PROCESSING;
    }
    
    // 处理当前任务
    if (current_task_->type == TaskType::BUFFER) { // 处理缓冲区类型任务
        char buffer[1024];
        while (outputbuffer_.size() > 0) {
            memset(buffer, 0, sizeof(buffer));
            int writeonce = outputbuffer_.putmessage(buffer, sizeof(buffer));
            ssize_t sent = ::send(cltsock_->get_fd(), buffer, writeonce, 0);
            if (sent < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return; // 暂时无法发送，等待下一次可写事件
                else { // 发送错误，处理错误(基本不进入此分支)
                    current_task_->status = TaskStatus::FAILED;
                    error_conn();
                    return;
                }
            }
            if (outputbuffer_.size() == 0) {
                current_task_->status = TaskStatus::COMPLETED;
                current_task_ = nullptr;
                send_task(); // 继续处理下一个任务
                return;
            }
        }
    } 
    
    else if (current_task_->type == TaskType::FILE) { // 处理文件类型任务
        ssize_t sent = sendfile(cltsock_->get_fd(), current_task_->file_fd, &current_task_->file_offset, current_task_->file_size);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return; // 暂时无法发送，等待下一次可写事件
            else { // 发送错误，处理错误
                close(current_task_->file_fd); // 关闭文件描述符
                current_task_->status = TaskStatus::FAILED;
                error_conn();
                return;
            }
        }
        current_task_->file_size -= sent;
        if (current_task_->file_size == 0) { // 文件发完
            close(current_task_->file_fd); // 关闭文件描述符
            current_task_->status = TaskStatus::COMPLETED;
            current_task_ = nullptr;
            send_task(); // 继续处理下一个任务
            return;
        }
    }
}

void Connection::addtask_toIOthread(std::shared_ptr<SendTask> task) {
    if (disconnect_ == true) {
        printf("connection(fd = %d) is disconnected, sending is cancelled.\n", cltsock_->get_fd());
        return;
    }
    if (loop_->in_loopthread()) addtask_inIOthread(task);
    else loop_->addtask([this, task]() { this->addtask_inIOthread(task); });
}

void Connection::addtask_inIOthread(std::shared_ptr<SendTask> task) {
    // 根据任务类型进行分支处理
    if (task->type == TaskType::BUFFER) {
        // 处理缓冲区类型任务
        if (task->buffer_data) {
            outputbuffer_.append_withsep(task->buffer_data->data(), task->buffer_data->size());
        }
    }
    // 文件类型任务不需要提前处理，直接添加到队列
    
    // 将任务添加到发送任务队列
    send_tasks_.push(task);
    conn_channel_->enablewriting();
}

void Connection::set_recvcallback(std::function<void(spConnection, std::shared_ptr<std::string>)> func) { recv_callback_ = func; }
void Connection::set_sendcallback(std::function<void(spConnection)> func) { send_callback_ = func; }
void Connection::set_closeconncallback(std::function<void(spConnection)> func) { closeconn_callback_ = func; }
void Connection::set_errorconncallback(std::function<void(spConnection)> func) { errorconn_callback_ = func; }

// Keep-Alive 相关方法
void Connection::set_keep_alive(bool keep_alive) { keep_alive_ = keep_alive; }
bool Connection::is_keep_alive() const { return keep_alive_; }
void Connection::increment_request_count() { request_count_++; }
bool Connection::should_close() const { return !keep_alive_ || request_count_ >= max_requests_; }
