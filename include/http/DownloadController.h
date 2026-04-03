#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"

class DownloadController {
public:
    DownloadController();
    ~DownloadController();
    
    // 处理文件下载请求
    HttpResponse handleFileDownload(const HttpRequest& request, const std::string& username);
    
private:
    bool validatePath(const std::string& path); // 验证路径，防止路径遍历攻击
};