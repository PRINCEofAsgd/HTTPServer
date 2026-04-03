#pragma once
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>

// 地址信息转换类，负责将输入的各种不同形式 ip / port / sockaddr_in，转化为函数可用的 sockaddr 或人类可读形式的 ip / port，本身不存储地址信息
class InetAddress {
private:
    sockaddr_in addr_in;

public:
    InetAddress();
    InetAddress(const std::string in_ip, const std::string in_port);
    InetAddress(const sockaddr_in in_addr);
    ~InetAddress();

    // 返回 sockaddr_in 结构体中各个参数
    const sockaddr* get_addr() const;
    const char* get_ip() const;
    const unsigned short get_port() const;
    const socklen_t get_len() const;

    // 设置 sockaddr_in 为传入参数
    // 由于 Socket 类中 accept4() 函数需要传入一个已有的 sockaddr 地址，此处无法直接构造，只能先声明再设置
    void set_addr(const sockaddr_in in_addr);
}; 
