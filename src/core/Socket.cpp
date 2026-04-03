#include "../../include/core/Socket.h"

int Socket::create_nonblocksock() {
    int listenfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (listenfd == -1) {
        printf("%s:%s:%d listenfd create error:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        exit(-1);
    }
    return listenfd;
}

Socket::Socket(int fd) : fd_(fd) {}
Socket::~Socket() { close(fd_); }

int Socket::get_fd() const { return fd_; }
std::string Socket::get_ip() const { return ip_; }
uint16_t Socket::get_port() const { return port_; }
void Socket::setaddr(std::string ip, uint16_t port) {
    ip_ = ip;
    port_ = port;
}

void Socket::set_reuseaddr(bool on) {
    int optval = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));    
}
void Socket::set_reuseport(bool on) {
    int optval = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}
void Socket::set_tcpnodelay(bool on) {
    int optval = on ? 1 : 0;
    setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}
void Socket::set_keepalive(bool on) {
    int optval = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}

void Socket::bind(const InetAddress& servaddr) {
    if (::bind(fd_, servaddr.get_addr(), servaddr.get_len()) == -1) {
        printf("%s:%s:%d bind failed:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        close(fd_);
        exit(-1);
    };
    setaddr(servaddr.get_ip(), servaddr.get_port());
}

void Socket::listen(int quenum) {
    if (::listen(fd_, quenum) == -1) {
        printf("%s:%s:%d listen failed:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        close(fd_);
        exit(-1);
    }
}

// 从等待队列中非阻塞地取出一个 clientsocket 连接，将其绑定的地址存入传入的参数地址，将其 fd 返回
int Socket::accept(InetAddress& cltaddr) { 
    sockaddr_in peeraddr;
    socklen_t peerlen = sizeof(peeraddr);
    int clientfd = ::accept4(fd_, (sockaddr*)&peeraddr, &peerlen, SOCK_NONBLOCK);
    cltaddr.set_addr(peeraddr);
    return clientfd;
}
