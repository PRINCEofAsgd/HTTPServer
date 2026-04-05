#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "AuthorMiddleWare.h"

class DownloadController {
private:
    AuthorMiddleWare* authorMiddleWare_;
    bool validatePath(const std::string& path); // 验证路径，防止路径遍历攻击

public:
    DownloadController(AuthorMiddleWare* authorMiddleWare);
    ~DownloadController();
    
    // 处理文件下载请求
    HttpResponse handleFileDownload(const HttpRequest& request);
};