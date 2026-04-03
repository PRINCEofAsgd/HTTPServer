#include "../../include/core/InetAddress.h"

InetAddress::InetAddress() { memset(&addr_in, 0, sizeof(sockaddr_in)); }
InetAddress::InetAddress(const std::string in_ip, const std::string in_port) {
    memset(&addr_in, 0, sizeof(sockaddr_in));
    addr_in.sin_family = PF_INET;
    addr_in.sin_addr.s_addr = inet_addr(in_ip.c_str());
    addr_in.sin_port = htons(atoi(in_port.c_str()));
}
InetAddress::InetAddress(const sockaddr_in in_addr) : addr_in(in_addr) {}
InetAddress::~InetAddress() {}

const sockaddr* InetAddress::get_addr() const { return (sockaddr*)&addr_in; }
const char* InetAddress::get_ip() const { return inet_ntoa(addr_in.sin_addr); }
const unsigned short InetAddress::get_port() const { return ntohs(addr_in.sin_port); }
const socklen_t InetAddress::get_len() const { return sizeof(addr_in); }

void InetAddress::set_addr(const sockaddr_in in_addr) { addr_in = in_addr; }
