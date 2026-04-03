#pragma once
#include <vector>
#include <memory>

class Buffer {
private:
    std::vector<char> buf_;
    size_t read_ptr_;           // 读指针，指向当前可读位置
    size_t write_ptr_;          // 写指针，指向当前可写位置
    const uint16_t sep_;        // 报文的分隔符: [0] 无分隔符(固定长度、视频会议); [1] 四字节的报头; [2] "\r\n\r\n" 分隔符(http协议)
    size_t max_message_size_;   // 单个消息的最大长度，防止错误/恶意数据导致缓冲区无限延伸耗尽内存

public:
    Buffer(uint16_t sep = 0, size_t max_message_size = 64 * 1024);
    ~Buffer();

    const char* data() const;         // 返回 C 风格字符串类型数据
    std::string sdata() const;
    size_t size() const;              // 返回字符串总长
    void clear();                                           // 清空缓冲区
    
    void append(const char* data, size_t len);              // 给缓冲区添加消息
    void erase(size_t pos, size_t len);                     // 删除缓冲区指定长度消息
    
    // 从缓冲区中获取单个消息，存储在 message 中，返回是否成功获取到完整消息(接收缓冲区用，根据 sep_ 分隔符处理不同形式的消息)
    bool getmessage(std::shared_ptr<std::string> message); 
    
    bool append_withsep(const char* data, size_t len);      // 给缓冲区添加带头部的消息(发送缓冲区使用)
    int putmessage(char* message, size_t max_len);          // 把数据分块从发送缓冲区发出去(发送缓冲区使用)，max_len 表示最大发送长度
};
