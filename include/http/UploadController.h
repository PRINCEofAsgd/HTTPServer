#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"

class UploadController {
public:
    UploadController();
    ~UploadController();
    
    // 处理文件上传请求
    HttpResponse handleFileUpload(const HttpRequest& request, const std::string& username);
};