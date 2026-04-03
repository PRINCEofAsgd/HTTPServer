// 心跳响应处理器
#pragma once

#include "HandlerRouter.h"

class HeartBeatHandler : public ResponseHandler {
public:
    HeartBeatHandler();
    ~HeartBeatHandler();
    
    void handle(const HttpResponse& response) override; // 处理心跳响应
};
