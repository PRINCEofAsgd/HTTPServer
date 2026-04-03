// 文件响应处理器
#pragma once

#include "HandlerRouter.h"

class FileHandler : public ResponseHandler {
private:
    void handle_file(const HttpResponse& response);
    void handle_html(const HttpResponse& response);

public:
    void handle(const HttpResponse& response) override;
};