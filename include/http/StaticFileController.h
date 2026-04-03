#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"

class StaticFileController {
public:
    StaticFileController();
    ~StaticFileController();
    
    // 处理静态文件获取请求
    HttpResponse handleStaticFile(const HttpRequest& request);
    
private:
    bool validatePath(const std::string& path); // 验证路径，防止路径遍历攻击
};