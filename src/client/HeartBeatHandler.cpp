// 心跳响应处理器实现
#include "../../include/client/HeartBeatHandler.h"
#include <iostream>
#include <time.h>

HeartBeatHandler::HeartBeatHandler() {}
HeartBeatHandler::~HeartBeatHandler() {}

void HeartBeatHandler::handle(const HttpResponse& response) {
    // 检查心跳响应状态
    // 心跳响应正常，更新最后活跃时间
    if (response.getStatusCode() == 200) {
        std::cout << "[心跳检测] 心跳响应正常" << std::endl;
        // 为解耦，ClientConn 中保留基础连接管理功能，这里需要通过全局客户端实例更新活跃时间，这里暂时只打印日志
    } 
    // 心跳响应异常，需要重建连接
    else {
        std::cerr << "[心跳检测] 心跳响应异常，状态码: " << response.getStatusCode() << std::endl;
        // 这里同样需要通过全局客户端实例来处理重建连接
    }
}
