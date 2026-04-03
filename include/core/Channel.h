#pragma once
#include "EventLoop.h"
#include "Socket.h"
#include <functional>

class Channel {
private:
    int fd_ = -1;                               // 封装事件的描述符
    uint32_t eventype_ = 0;                     // 事件类型
    EventLoop* loop_;    // 监听事件的 loop 循环，内含 epoll 句柄，Channel 需加入此循环监听，不拥有循环对象，只引用它循环监听自己
    bool inepoll_ = false;                      // Channel 是否加入 epoll 句柄
    uint32_t rtneventype_ = 0;                  // 返回事件，即封装的事件触发的事件
    std::function<void()> read_callback_;       // 处理读事件(Acceptor 有新连接，或 Connection 有通讯数据)的回调函数声明
    std::function<void()> write_callback_;      // 处理写事件
    std::function<void()> close_callback_;      // 处理连接关闭事件的回调函数声明
    std::function<void()> error_callback_;      // 处理连接错误事件的回调函数声明

public:
    Channel(int fd, EventLoop* loop);
    ~Channel();

    // 返回私有成员
    int get_fd();
    uint32_t get_evtype();
    bool get_inepoll();
    uint32_t get_rtevtype();

    // 设置事件参数
    void set_evtype_et();
    void set_inepoll(bool inepoll = true);
    void set_rtevtype(uint32_t rtnev);

    void enablereading();   // 注册读事件: 调用 Loop 类函数将 Channel 加入所属循环监听读事件(本质为 Loop 类再调用 Epoll 类将其加入 Epoll 句柄)
    void disablereading();  // 取消写事件
    void enablewriting();   // 注册写事件
    void disablewriting();  // 取消写事件
    void disableall();      // 取消所有事件
    void remove();          // 将 Channel 从所属循环中移除

    // 设置回调函数，上层类可调用设置，使此类的回调函数回调上层类的函数
    void set_readcallback(std::function<void()> fnc);
    void set_writecallback(std::function<void()> fnc);
    void set_closecallback(std::function<void()> fnc);
    void set_errorcallback(std::function<void()> fnc);

    void handle();      // 事件处理函数，根据不同触发事件的类型调用不同的回调函数，回调不同的上层类对象处理，实现职责解耦
};
