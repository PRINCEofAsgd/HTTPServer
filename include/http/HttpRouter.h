#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <string>
#include <unordered_map>
#include <functional>

typedef std::function<HttpResponse(const HttpRequest&)> RouteHandler; // 定义处理函数类型

typedef struct { // 路由键结构体，用于存储方法和路径的组合
    std::string method;
    std::string path;
} RouteKey;

namespace std {
    template<> // 为RouteKey特化哈希函数
    struct hash<RouteKey> {
        size_t operator()(const RouteKey& key) const {
            return hash<string>()(key.method + key.path);
        }
    };

    template<> // 为RouteKey特化相等比较函数
    struct equal_to<RouteKey> {
        bool operator()(const RouteKey& lhs, const RouteKey& rhs) const {
            return lhs.method == rhs.method && lhs.path == rhs.path;
        }
    };
}

class HttpRouter {
private:
    std::unordered_map<RouteKey, RouteHandler> routes_; // 路由映射表，键为方法和路径的组合，值为对应的处理函数

public:
    HttpRouter();
    ~HttpRouter();

    void registerRoute(const std::string& method, const std::string& path, RouteHandler handler); // 注册路由
    HttpResponse handleRequest(const HttpRequest& request);             // 处理请求，根据方法和路径匹配对应的处理函数，返回响应

};