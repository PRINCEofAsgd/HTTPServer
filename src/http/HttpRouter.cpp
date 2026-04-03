#include "../../include/http/HttpRouter.h"

HttpRouter::HttpRouter() {}

HttpRouter::~HttpRouter() {}

void HttpRouter::registerRoute(const std::string& method, const std::string& path, RouteHandler handler) {
    RouteKey key = {method, path};
    routes_[key] = handler;
}

HttpResponse HttpRouter::handleRequest(const HttpRequest& request) {
    RouteKey key = {request.getMethod(), request.getPath()};
    auto it = routes_.find(key);
    
    if (it != routes_.end()) return it->second(request); // 找到匹配的路由，调用对应的处理函数
    else { // 未找到匹配的路由，返回404错误
        HttpResponse response(404);
        response.addHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>404 Not Found</h1></body></html>");
        return response;
    }
}