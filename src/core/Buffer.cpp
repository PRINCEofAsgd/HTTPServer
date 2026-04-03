#include "../../include/core/Buffer.h"
#include <cstring>
#include <string>
#include <arpa/inet.h>

Buffer::Buffer(uint16_t sep, size_t max_message_size) : 
buf_(), read_ptr_(0), write_ptr_(0), sep_(sep), max_message_size_(max_message_size) {
    buf_.reserve(1024); // 预分配一些空间，避免频繁扩容
}
Buffer::~Buffer() {}

const char* Buffer::data() const { return buf_.data() + read_ptr_; }
std::string Buffer::sdata() const { return std::string(data(), size()); }
size_t Buffer::size() const { return write_ptr_ - read_ptr_; }

void Buffer::clear() { 
    read_ptr_ = 0;
    write_ptr_ = 0;
    if (buf_.capacity() > 1024 * 1024) { // 保留一些空间，避免频繁分配
        buf_.clear();
        buf_.reserve(1024);
    }
}

void Buffer::append(const char* data, size_t len) {
    if (write_ptr_ + len > buf_.size()) { // 检查是否需要扩容
        if (read_ptr_ > 0) { // 如果读指针前面有足够空间，先进行数据移动
            memmove(buf_.data(), buf_.data() + read_ptr_, this->size());
            write_ptr_ -= read_ptr_;
            read_ptr_ = 0;
        }
        
        if (write_ptr_ + len > buf_.capacity()) // 确保有足够空间
            buf_.reserve(std::max(buf_.capacity() * 2, write_ptr_ + len));
        
        buf_.resize(write_ptr_ + len); // 扩容
    }
    
    memcpy(buf_.data() + write_ptr_, data, len); // 复制数据
    write_ptr_ += len;
}

bool Buffer::append_withsep(const char* data, size_t len) {
    size_t total_size = len;
    if (sep_ == 1) total_size += 4; // 加上4字节长度头部
    else if (sep_ == 2) {} // sep_ == 2 时，直接使用len，不需要额外头部
    
    if (total_size > max_message_size_) return false; // 检查是否超过最大消息大小
    
    if (write_ptr_ + total_size > buf_.size()) { // 检查是否需要扩容
        if (read_ptr_ > 0) { // 如果读指针前面有足够空间，先进行数据移动
            memmove(buf_.data(), buf_.data() + read_ptr_, this->size());
            write_ptr_ -= read_ptr_;
            read_ptr_ = 0;
        }
        
        if (write_ptr_ + total_size > buf_.capacity()) // 确保有足够空间
            buf_.reserve(std::max(buf_.capacity() * 2, write_ptr_ + total_size));
        
        buf_.resize(write_ptr_ + total_size); // 扩容
    }
    
    // 写入数据
    if (sep_ == 0) {
        memcpy(buf_.data() + write_ptr_, data, len);
        write_ptr_ += len;
    } 
    
    else if (sep_ == 1) {
        uint32_t netlen = htonl(len);
        memcpy(buf_.data() + write_ptr_, &netlen, 4);
        memcpy(buf_.data() + write_ptr_ + 4, data, len);
        write_ptr_ += 4 + len;
    } 
    
    else if (sep_ == 2) { // HTTP报文处理，直接写入数据，不需要添加额外分隔符
        memcpy(buf_.data() + write_ptr_, data, len);
        write_ptr_ += len;
    }
    
    return true;
}

void Buffer::erase(size_t pos, size_t len) {
    if (pos + len > this->size()) clear(); // 删除位置和长度超过缓冲区大小，直接清空缓冲区
    else {
        read_ptr_ += pos + len;
        if (read_ptr_ >= write_ptr_) clear(); // 如果缓冲区为空，重置指针
    }
}

bool Buffer::getmessage(std::shared_ptr<std::string> message) {
    if (size() == 0) return false;
    
    if (sep_ == 0) {
        message->assign(data(), size());
        clear();
    } 
    
    else if (sep_ == 1) {
        if (size() < 4) return false; // 单个消息长度小于 4，消息长度首部不完整，返回 false
        
        uint32_t netlen;
        memcpy(&netlen, data(), 4); // 从缓冲区中获取 4 字节的单个消息的网络字节序长度首部
        uint32_t hostlen = ntohl(netlen); // 将网络字节序转换为主机字节序，得到程序可识别的单个消息的长度
        
        if (hostlen > max_message_size_) {
            clear(); // 单个消息长度超过最大消息大小，可能是报文长度首部解析错误或恶意长报文
            return false;
        }
        
        if (size() < hostlen + 4) return false; // 单个消息长度小于 hostlen + 4，消息不完整，返回 false
        
        message->assign(data() + 4, hostlen); // 从缓冲区中获取单个消息内容，存储在 message 中
        erase(0, hostlen + 4); // 从缓冲区中删除已获取的消息
    } 
    
    else if (sep_ == 2) {
        // HTTP报文处理，按\r\n\r\n分隔符处理
        const char* data = this->data();
        size_t size = this->size();
        
        // 查找\r\n\r\n分隔符
        for (size_t i = 0; i < size - 3; ++i) {
            if (data[i] == '\r' && data[i + 1] == '\n' && data[i + 2] == '\r' && data[i + 3] == '\n') {
                // 提取请求头
                std::string headers(data, i + 4);
                
                // 检查是否有Content-Length头
                size_t contentLength = 0;
                size_t contentLengthPos = headers.find("Content-Length:");
                if (contentLengthPos != std::string::npos) {
                    size_t endPos = headers.find("\r\n", contentLengthPos);
                    if (endPos != std::string::npos) {
                        std::string lengthStr = headers.substr(contentLengthPos + 16, endPos - (contentLengthPos + 16));
                        // 去除空格
                        lengthStr.erase(0, lengthStr.find_first_not_of(' '));
                        lengthStr.erase(lengthStr.find_last_not_of(' ') + 1);
                        contentLength = std::stoul(lengthStr);
                    }
                }
                
                // 检查是否有足够的数据
                if (i + 4 + contentLength > size) {
                    return false; // 消息不完整
                }
                
                // 提取完整的HTTP报文
                message->assign(data, i + 4 + contentLength);
                erase(0, i + 4 + contentLength);
                return true;
            }
        }
        
        // 没有找到完整的HTTP报文
        return false;
    }
    return true;
}

int Buffer::putmessage(char* message, size_t max_len) {
    size_t available = size();
    size_t send_len = std::min(available, max_len);
    
    if (send_len > 0) {
        memcpy(message, data(), send_len);
        read_ptr_ += send_len;
        if (read_ptr_ >= write_ptr_) clear(); // 如果缓冲区为空，重置指针
        return send_len;
    } 
    else return 0;
}
