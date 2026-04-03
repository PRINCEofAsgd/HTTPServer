#include "../../include/core/TcpServer.h"

// 构造函数中创建循环对象，并使用提供的地址创建 Acceptor 对象，设置新连接回调函数和超时回调函数
TcpServer::TcpServer(const std::string& ip, const std::string& port, const int threadnum_IO, 
    const int eptime_out, const int time_out, const int time_interval, const int sep, const int max_requests) : 
    mainloop_(std::make_unique<EventLoop>(eptime_out, true, time_out, time_interval)), acc_(mainloop_.get(), ip, port), sep_(sep),
    threadnum_(threadnum_IO), threadpool_(threadnum_, "IO"), time_out_(time_out), max_requests_(max_requests)
{
    mainloop_->set_eptimeoutcallback([this](EventLoop* loop) { this->handle_eptimeout(loop); }); 
    acc_.set_newconncallback([this](std::unique_ptr<Socket> cltsock) { this->handle_newconn(std::move(cltsock)); });
    // 创建 IO 事件线程池，EventLoop::run() 内部是一个无限循环，线程开始执行就会一直循环，不会再去线程池抢其他任务，自然就保证了一个线程绑定一个 EventLoop
    for (int i = 0; i < threadnum_; ++i) {
        subloops_.emplace_back(std::make_unique<EventLoop>(eptime_out, false, time_out, time_interval)); 
        subloops_[i]->set_eptimeoutcallback([this](EventLoop* loop) { this->handle_eptimeout(loop); });
        subloops_[i]->set_timeoutcallback([this](spConnection conn) { this->handle_removeconn(conn); });
        threadpool_.addtask([this, i]() { this->subloops_[i]->run(); });
    }
}

TcpServer::~TcpServer() { }

void TcpServer::start() { mainloop_->run(); }

void TcpServer::stop() {
    mainloop_->stop();
    printf("mainloop has been stopped.\n");
    for (int i = 0; i < threadnum_; ++i) {
        subloops_[i]->stop();
        printf("subloop %d has been stopped.\n", i);
    }
    threadpool_.stop();
    printf("IO threads have been stopped.\n");
}

void TcpServer::handle_newconn(std::unique_ptr<Socket> cltsock) {
    spConnection conn = std::make_shared<Connection>(subloops_[cltsock->get_fd() % threadnum_].get(), std::move(cltsock), sep_, 4 * 1024 * 1024, max_requests_); 
    conn->set_closeconncallback([this](spConnection c) { this->handle_closeconn(c); });
    conn->set_errorconncallback([this](spConnection c) { this->handle_errorconn(c); });
    conn->set_recvcallback([this](spConnection c, std::shared_ptr<std::string> msg) { this->handle_recvmessage(c, msg); });
    conn->set_sendcallback([this](spConnection c) { this->handle_sendmessage(c); });
    
    {   // 主线程
        std::lock_guard<std::mutex> lock(conn_mutex);               // TcpServer 对象图中 conn 可能由主线程、IO 线程多个线程争抢修改，需要加锁保护
        conns_[conn->get_fd()] = conn;
    }
    subloops_[conn->get_fd() % threadnum_]->handle_newconn(conn);   // EventLoop 对象图中 conn 的保护由其自己的函数加锁
    if (newconn_callback_) newconn_callback_(conn);
}

void TcpServer::handle_closeconn(spConnection conn) {   // 主线程
    if (closeconn_callback_) closeconn_callback_(conn);
    std::lock_guard<std::mutex> lock(conn_mutex);
    conns_.erase(conn->get_fd()); 
}

void TcpServer::handle_errorconn(spConnection conn) {   // 主线程
    if (errorconn_callback_) errorconn_callback_(conn);
    std::lock_guard<std::mutex> lock(conn_mutex);
    conns_.erase(conn->get_fd()); 
}

void TcpServer::handle_removeconn(spConnection conn) {  // IO 线程
    if (conns_.find(conn->get_fd()) == conns_.end()) return;    // EventLoop 定时监测监听可能超时，若 conn 已主动关闭则返回
    if (removeconn_callback_) removeconn_callback_(conn);
    std::lock_guard<std::mutex> lock(conn_mutex);
    conns_.erase(conn->get_fd());
}

void TcpServer::handle_recvmessage(spConnection conn, std::shared_ptr<std::string> message) { if (recv_callback_) recv_callback_(conn, message); }
void TcpServer::handle_sendmessage(spConnection conn) { if (send_callback_) send_callback_(conn); }
void TcpServer::handle_eptimeout(EventLoop* loop) { if (eptimeout_callback_) eptimeout_callback_(loop); }

void TcpServer::set_newconncallback(std::function<void(spConnection)> func) { newconn_callback_ = func; }
void TcpServer::set_closeconncallback(std::function<void(spConnection)> func) { closeconn_callback_ = func; }
void TcpServer::set_errorconncallback(std::function<void(spConnection)> func) { errorconn_callback_ = func; }
void TcpServer::set_removeconncallback(std::function<void(spConnection)> func) { removeconn_callback_ = func; }
void TcpServer::set_recvcallback(std::function<void(spConnection, std::shared_ptr<std::string>)> func) { recv_callback_ = func; }
void TcpServer::set_sendcallback(std::function<void(spConnection)> func) { send_callback_ = func; }
void TcpServer::set_eptimeoutcallback(std::function<void(EventLoop*)> func) { eptimeout_callback_ = func; }
