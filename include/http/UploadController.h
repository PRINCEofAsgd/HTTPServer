#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "AuthorMiddleWare.h"

class UploadController {
private:
    AuthorMiddleWare* authorMiddleWare_;

public:
    UploadController(AuthorMiddleWare* authorMiddleWare);
    ~UploadController();
    
    // 处理文件上传请求
    HttpResponse handleFileUpload(const HttpRequest& request);
};