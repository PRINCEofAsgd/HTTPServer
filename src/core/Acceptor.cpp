#include "../../include/core/Acceptor.h"

// 构造函数中创建监听套接字和处理器，并加入循环进行监听
Acceptor::Acceptor(EventLoop* loop, const std::string& ip, const std::string& port) : 
loop_(loop), servsock_(Socket::create_nonblocksock()), acc_channel_(servsock_.get_fd(), loop_) {    
    // 套接字设置
    servsock_.set_reuseaddr(true);
    servsock_.set_reuseport(true);
    servsock_.set_tcpnodelay(true);
    servsock_.set_keepalive(true);
    InetAddress serveraddr = InetAddress(ip, port);
    servsock_.bind(serveraddr);
    servsock_.listen(128);
    acc_channel_.set_readcallback(std::bind(&Acceptor::newconn_callback, this));   // 设置处理器触发后回调函数回调本层 Acceptor 对象的函数处理新连接
    acc_channel_.enablereading();                                                  // 调用 channel 的函数注册 读事件 的监听
    // 打印建立接收端成功信息，因为接收端只建立一次，也是底层重要步骤，写很多回调多此一举，所以小耦合可以忽略
    printf("launch acceptor(fd = %d, ip = %s, port = %d) success.\n", servsock_.get_fd(), servsock_.get_ip().c_str(), servsock_.get_port());
}

Acceptor::~Acceptor() { }

void Acceptor::newconn_callback() { // 接收到新连接后，将接收到的套接字和地址回调给 TcpServer 统一创建管理 Connection 类
    InetAddress cltaddr;
    std::unique_ptr<Socket> cltsock = std::make_unique<Socket>(servsock_.accept(cltaddr));
    cltsock->setaddr(cltaddr.get_ip(), cltaddr.get_port());
    newconn_callback_(std::move(cltsock));
}

void Acceptor::set_newconncallback(std::function<void(std::unique_ptr<Socket>)> func) { newconn_callback_ = func; }
