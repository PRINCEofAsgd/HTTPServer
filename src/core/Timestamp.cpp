#include "../../include/core/Timestamp.h"
#include <cstring>

Timestamp::Timestamp() { sec_since_epoch_ = time(0); }      // 取系统当前时间设置成员
Timestamp::Timestamp(int64_t sec_since_epoch) : sec_since_epoch_(sec_since_epoch) {}
Timestamp::~Timestamp() { }

void Timestamp::set_now() { sec_since_epoch_ = time(0); }   // 更新成员为系统当前时间
Timestamp Timestamp::now() { return Timestamp(); }          // 返回成员为当前时间的对象

time_t Timestamp::get_toint() const { return sec_since_epoch_; }

std::string Timestamp::get_tostring() const { 
    char buffer[32];
    memset(&buffer, 0, sizeof(buffer));
    tm* tm_time = localtime(&sec_since_epoch_);
    snprintf(buffer, 20, "%4d-%02d-%02d %02d:%02d:%02d",
             tm_time->tm_year + 1900,
             tm_time->tm_mon + 1,
             tm_time->tm_mday,
             tm_time->tm_hour,
             tm_time->tm_min,
             tm_time->tm_sec);
    return buffer;
}
