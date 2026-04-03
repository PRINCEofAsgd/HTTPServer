#pragma once
#include <vector>
#include <sys/epoll.h>

class Channel;

class Epoll {
private:
    int fd_;                                    // epoll句柄，在构造函数中创建。
    static const int MaxEvents_ = 128;          // epoll_wait() 返回事件数组的大小。
    epoll_event return_events[MaxEvents_];      // 存放 epoll_wait() 返回事件的数组，在构造函数中分配内存。

public:
    static int create_epoll();
    Epoll(int fd = -1);
    ~Epoll();
    
    void update_channel(Channel* ch);                       // 将传入的 Channel 封装的事件加入 epoll 句柄监听
    void remove_channel(Channel* ch);                       // 将传入的 Channel 封装的事件从 epoll 句柄中移除
    std::vector<Channel*> loop(int timeout = -1);           // epoll_wait() 句柄中的事件，将触发事件的 Channel 容器返回
}; 
