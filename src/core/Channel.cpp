#include "../../include/core/Channel.h"
#include "../../include/core/Epoll.h"

Channel::Channel(int fd, EventLoop* loop) : fd_(fd), loop_(loop) {}
Channel::~Channel() {}

int Channel::get_fd() { return fd_; }
uint32_t Channel::get_evtype() { return eventype_; }
bool Channel::get_inepoll() { return inepoll_; }
uint32_t Channel::get_rtevtype() { return rtneventype_; }

void Channel::set_evtype_et() { eventype_ |= EPOLLET; }
void Channel::set_inepoll(bool inepoll) { inepoll_ = inepoll; }
void Channel::set_rtevtype(uint32_t rtnev) { rtneventype_ = rtnev; }

void Channel::enablereading() {
    eventype_ |= EPOLLIN;
    loop_->update_channel(this);
}
void Channel::disablereading() {
    eventype_ &= ~EPOLLIN;
    loop_->update_channel(this);
}
void Channel::enablewriting() {
    eventype_ |= EPOLLOUT;
    loop_->update_channel(this);
}
void Channel::disablewriting() {
    eventype_ &= ~EPOLLOUT;
    loop_->update_channel(this);
}
void Channel::disableall() {
    eventype_ = 0;
    loop_->update_channel(this);
}
void Channel::remove() {
    // disableall();
    loop_->remove_channel(this);
}

void Channel::set_readcallback(std::function<void()> func) { read_callback_ = func; }
void Channel::set_writecallback(std::function<void()> func) { write_callback_ = func; }
void Channel::set_closecallback(std::function<void()> func) { close_callback_ = func; }
void Channel::set_errorcallback(std::function<void()> func) { error_callback_ = func; }

void Channel::handle() {
    if (rtneventype_ & EPOLLHUP) close_callback_();                 // 处理套接字关闭事件
    else if (rtneventype_ & (EPOLLIN|EPOLLPRI) ) read_callback_();  // 处理读事件、优先事件
    else if (rtneventype_ & EPOLLOUT) write_callback_();            // 处理写事件
    else error_callback_();                                         // 处理错误事件
}
