// 响应处理器路由类
#pragma once

#include "../http/HttpResponse.h"
#include "../core/Buffer.h"
#include <string>
#include <functional>
#include <unordered_map>

// 前向声明
class HttpRequest;

// 响应处理器接口
class ResponseHandler {
public:
    virtual ~ResponseHandler() = default;
    virtual void handle(const HttpResponse& response) = 0;
};

// 请求类型枚举
enum class ReqType {
    HEARTBEAT,       // 心跳检测
    LOGIN,           // 登录
    REGISTER,        // 注册
    GET_STATIC,      // 获取静态资源
    DOWNLOAD_FILE,   // 下载个人文件
    UPLOAD_FILE      // 上传个人文件
};

// 请求上下文结构体
struct RequestContext {
    ReqType reqType;                                    // 请求的业务类型
    std::function<void(const HttpResponse&)> callback;   // 回调函数
};

// 处理器路由类
class HandlerRouter {
private:
    std::unordered_map<int, RequestContext> routeMap; // 基于 X-Request-ID 的路由映射表
    void printResponseMetadata(const HttpResponse& response);   // 输出响应报文元数据信息
    
public:
    HandlerRouter();
    ~HandlerRouter();
    
    void RegisterRouter(int requestId, const RequestContext& context); // 注册基于请求 ID 的路由
    void handleResponse(const HttpResponse& response);          // 处理响应
};