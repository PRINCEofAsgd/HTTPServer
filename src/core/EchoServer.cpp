#include "../../include/core/EchoServer.h"
#include "../../include/core/SendTask.h"
#include <sys/syscall.h>

EchoServer::EchoServer(TcpServer* tcpserver, int threadnum_comp) : tcpserver_(tcpserver), threadpool_(threadnum_comp, "COMP") {
    // 设置 TcpServer 不同通讯事件回调函数，事件有对应的业务可回调本层业务层处理，没有则注释掉业务即可忽略事件、不回调
    tcpserver_->set_newconncallback([this](spConnection conn) { this->handle_newconn(conn); });
    tcpserver_->set_closeconncallback([this](spConnection conn) { this->handle_closeconn(conn); });
    tcpserver_->set_errorconncallback([this](spConnection conn) { this->handle_errorconn(conn); });
    tcpserver_->set_removeconncallback([this](spConnection conn) { this->handle_removeconn(conn); });
    tcpserver_->set_recvcallback([this](spConnection conn, std::shared_ptr<std::string> msg) { this->handle_recv(conn, msg); });
    tcpserver_->set_sendcallback([this](spConnection conn) { this->handle_send (conn); });
    // tcpserver_->set_eptimeoutcallback([this](EventLoop* loop) { this->handle_eptimeout(loop); });
}
EchoServer::~EchoServer() { }

void EchoServer::start() { tcpserver_->start(); }
void EchoServer::stop() {
    threadpool_.stop();
    printf("comp threads have been stopped.\n");
}

void EchoServer::handle_newconn(spConnection conn) {
    printf("new connection(fd = %d, ip = %s, port = %d) success in main thread %ld.\n", conn->get_fd(), conn->get_ip().c_str(), conn->get_port(), syscall(SYS_gettid));
}
void EchoServer::handle_closeconn(spConnection conn) {
    printf("connection(fd = %d) disconnected in main thread %ld.\n", conn->get_fd(), syscall(SYS_gettid));
}
void EchoServer::handle_errorconn(spConnection conn) {
    printf("connection(fd = %d) error in main thread %ld.\n", conn->get_fd(), syscall(SYS_gettid));
}
void EchoServer::handle_removeconn(spConnection conn) {
    printf("connection(fd = %d) removed in IO thread %ld.\n", conn->get_fd(), syscall(SYS_gettid));
}
void EchoServer::handle_recv(spConnection conn, std::shared_ptr<std::string> message) {
    // printf("receive message(fd = %d, mes = %s) success in IO thread %ld.\n", conn->get_fd(), message->c_str(), syscall(SYS_gettid));
    // parse_json(); query_database(); call_microservice(); run_algorithm();
    // 以上是真实场景接收到数据后可能涉及的计算操作，可能耗时较长，加入 comp 线程池处理，避免阻塞 IO 线程
    threadpool_.addtask([this, conn, message]() { this->handle_comp(conn, message); });
    // 任务加入 comp 线程后，随着 IO 线程继续进行，message 可能修改或释放，故使用 mutable 关键字值传递 message
}
void EchoServer::handle_send(spConnection conn) {
    // printf("send message(fd = %d) success in IO thread %ld.\n\n", conn->get_fd(), syscall(SYS_gettid));
}
void EchoServer::handle_eptimeout(EventLoop* loop) {
    printf("epoll_wait() out of time %d seconds in IO thread %ld.\n", loop->get_timeout() / 1000, syscall(SYS_gettid));
}

void EchoServer::handle_comp(spConnection conn, std::shared_ptr<std::string> message) {
    // printf("handle_comp() in comp thread %ld.\n", syscall(SYS_gettid));
    std::shared_ptr<std::string> reply = std::make_shared<std::string>("reply: " + *message); // 生成回复消息，即回显业务的计算
    auto buffer_task = std::make_shared<SendTask>(TaskType::BUFFER);
    buffer_task->buffer_data = reply;
    conn->addtask_toIOthread(buffer_task);
}
