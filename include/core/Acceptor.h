#pragma once
#include "Channel.h"
#include <functional>
#include <memory>

class Acceptor {
private:
    EventLoop* loop_;        // TcpServer 的主体循环，Acceptor 需加入此循环监听，不拥有循环对象，只引用它循环监听自己
    Socket servsock_;                               // 接收对端连接的 listensocket
    Channel acc_channel_;                           // 接收对端连接的事件处理器
    std::function<void(std::unique_ptr<Socket>)> newconn_callback_;     // 处理新连接事件的回调函数声明

public:
    Acceptor(EventLoop* loop, const std::string& ip, const std::string& port);  // 使用提供的地址创建监听套接字和处理器，并加入循环进行监听
    ~Acceptor();

    void newconn_callback();    // 本层的回调函数，可被下层函数分化回调，也可回调上层函数实现解耦

    void set_newconncallback(std::function<void(std::unique_ptr<Socket>)> func);    // 设置回调函数
};
