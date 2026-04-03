#include "../../include/core/Epoll.h"
#include "../../include/core/Channel.h"
#include <cstring>

int Epoll::create_epoll() {
    int epollfd = epoll_create(1);
    if (epollfd == -1) {
        printf("%s:%s:%d epollfd create error:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        exit(-1);
    }
    return epollfd;
}

Epoll::Epoll(int fd) : fd_(fd) {}
Epoll::~Epoll() { close(fd_); }

void Epoll::update_channel(Channel* ch) {

    if (ch == nullptr) {
        printf("%s:%s:%d epoll ctl channel: ch is nullptr:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        return;
    }
    // 用存放了所有 epoll_event 成员的 channel 类对象 ch 的成员辅助创建 epoll_event 对象(因为 epoll_ctl 最终只支持 epoll_event)
    epoll_event ev_ctl;
    ev_ctl.events = ch->get_evtype();
    ev_ctl.data.ptr = ch;

    if (ch->get_inepoll() == false) {           // 如果 epoll 中未加入过该 channel, 用 EPOLL_CTL_ADD 加入
        if (epoll_ctl(fd_, EPOLL_CTL_ADD, ch->get_fd(), &ev_ctl) == -1) {
            printf("%s:%s:%d epoll_ctl() add event failed:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
            exit(-1);
        }
        ch->set_inepoll();
    }
    else {                                      // 如果 epoll 中已加入过 channell, 用 EPOLL_CTL_MOD 修改
        if (epoll_ctl(fd_, EPOLL_CTL_MOD, ch->get_fd(), &ev_ctl) == -1) {
            printf("%s:%s:%d epoll_ctl() modify event failed:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
            exit(-1);
        }
    }

}

void Epoll::remove_channel(Channel* ch) {
    if (ch == nullptr) {
        printf("%s:%s:%d epoll ctl channel: ch is nullptr:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        return;
    }
    if (ch->get_inepoll() == true) {            // 如果 epoll 中已加入过该 channel, 用 EPOLL_CTL_DEL 删除
        if (epoll_ctl(fd_, EPOLL_CTL_DEL, ch->get_fd(), nullptr) == -1) {
            printf("%s:%s:%d epoll_ctl() delete event failed:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
            exit(-1);
        }
        // 将Channel对象的inepoll_标志设置为false
        ch->set_inepoll(false);
    }
}

std::vector<Channel*> Epoll::loop(int timeout) {

    std::vector<Channel*> rtnchannels;
    memset(return_events, 0, sizeof(return_events));
    int fdnums = epoll_wait(fd_, return_events, MaxEvents_, timeout);

    if (fdnums == -1) {         // 返回失败
        printf("%s:%s:%d epoll_wait() failed:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        exit(-1);
    }

    else if (fdnums == 0) {     // 返回超时
        return rtnchannels;     // 返回空的事件容器
    }

    else {                      // 返回触发的事件数
        for (int i = 0; i < fdnums; ++i) {      // 遍历触发的事件(事件类型 + 封装事件信息的 Channel)
            Channel *single_ch = (Channel*)return_events[i].data.ptr;   // 取出封装触发事件的 Channel
            single_ch->set_rtevtype(return_events[i].events);           // 将返回事件封装在 Channel 中
            rtnchannels.push_back(single_ch);                           // 返回事件加入待处理事件容器
        }
    }

    return rtnchannels;

}
