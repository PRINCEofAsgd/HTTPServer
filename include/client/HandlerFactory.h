// HandlerFactory类，负责整合HttpContext和HttpRouter
#pragma once
#include <memory>
#include <thread>
#include "HandlerRouter.h"

class ClientConn;
using spClientConn = std::shared_ptr<ClientConn>;
class ClientLoop;
class HttpRequest;
class HttpResponse;
class HttpContext;
class HandlerRouter;
class JsonHandler;
class FileHandler;
class HeartBeatHandler;

// HandlerFactory类，负责整合HttpContext和HttpRouter
class HandlerFactory {
private:
    std::unique_ptr<ClientLoop> client_loop_;
    spClientConn client_conn_;
    std::thread loop_thread_;
    std::thread handler_thread_;
    std::unique_ptr<HttpContext> context_;      // HTTP上下文，用于解析响应
    std::unique_ptr<HandlerRouter> router_;     // 路由分发器，用于处理响应
    
    // Handler成员变量
    std::unique_ptr<JsonHandler> jsonHandler_;      // JSON响应处理器
    std::unique_ptr<FileHandler> fileHandler_;      // 文件响应处理器
    std::unique_ptr<HeartBeatHandler> heartBeatHandler_; // 心跳响应处理器

public:
    HandlerFactory();
    ~HandlerFactory();

    // 处理HTTP响应
    bool handleResponse(const std::string& message);
    void handleRequest(const HttpRequest& request);
    
    // 注册路由
    void RegisterRouter(int requestId, ReqType reqType);
};
