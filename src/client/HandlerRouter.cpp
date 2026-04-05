// 处理器路由类实现
#include "../../include/client/HandlerRouter.h"
#include "../../include/http/HttpContext.h"
#include "../../include/core/Buffer.h"
#include "../../include/core/SendTask.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

HandlerRouter::HandlerRouter() {}
HandlerRouter::~HandlerRouter() {
    routeMap.clear(); // 只清理映射，不删除处理器（由HandlerFactory管理）
}

// 输出响应报文元数据信息
void HandlerRouter::printResponseMetadata(const HttpResponse& response) {
    printf("\n=== 响应报文元数据 ===\n");
    printf("状态码: %d\n", response.getStatusCode());
    printf("HTTP版本: %s\n", response.getVersion().c_str());
    
    // 输出响应头信息
    printf("响应头:\n");
    const auto& headers = response.getHeaders();
    for (const auto& header : headers) 
        printf("  %s: %s\n", header.first.c_str(), header.second.c_str());
    
    // 输出响应时间戳
    /* time_t now = time(nullptr);
    struct tm* localTime = localtime(&now);
    char timeBuffer[80];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localTime);
    printf("响应时间: %s\n", timeBuffer);
    printf("=====================\n"); */
}

// 注册基于请求 ID 的路由
void HandlerRouter::RegisterRouter(int requestId, const RequestContext& context) {
    std::lock_guard<std::mutex> lock(routeMapMutex);
    routeMap[requestId] = context;
}

// 处理响应
void HandlerRouter::handleResponse(const HttpResponse& response) {
    // 首先调用 printResponseMetadata 方法输出响应报文元数据
    printResponseMetadata(response);
    
    // 从响应头中提取 X-Request-ID 字段
    std::string xRequestId = response.getXRequestId();
    
    if (xRequestId != "default-request-id") {
        try {
            int requestId = std::stoi(xRequestId);
            // 根据 X-Request-ID 查找对应的 RequestContext
            std::unique_lock<std::mutex> lock(routeMapMutex);
            auto ctxIt = routeMap.find(requestId);
            if (ctxIt != routeMap.end()) {
                auto callback = ctxIt->second.callback;
                routeMap.erase(ctxIt);  // 执行完后从路由映射表中移除
                lock.unlock();      // 提前释放锁，避免在回调中持有锁
                callback(response); // 执行回调函数
            } 
            else printf("\n未找到对应的请求 ID: %d\n", requestId);
        } catch (const std::invalid_argument& e) {
            printf("\n无效的 X-Request-ID 格式\n");
        } catch (const std::out_of_range& e) {
            printf("\nX-Request-ID 值超出范围\n");
        }
    } 
    else printf("\n响应头中未找到 X-Request-ID 字段\n");
}
