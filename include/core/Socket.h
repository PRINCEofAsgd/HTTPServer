#pragma once
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include "InetAddress.h"

// 存储套接字描述符和地址，设置和操作套接字的类
class Socket {
private:
    const int fd_;
    std::string ip_;
    uint16_t port_;

public:
    static int create_nonblocksock();
    Socket(int fd = -1);
    ~Socket();

    int get_fd() const;
    std::string get_ip() const;
    uint16_t get_port() const;
    void setaddr(std::string ip, uint16_t port);

    // 设置套接字选项
    void set_reuseaddr(bool on); 
    void set_reuseport(bool on); 
    void set_tcpnodelay(bool on);
    void set_keepalive(bool on); 

    // 套接字操作
    void bind(const InetAddress& servaddr); 
    void listen(int quenum = 128);
    int accept(InetAddress& cltaddr);
}; 
