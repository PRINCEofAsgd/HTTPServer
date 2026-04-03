#pragma once
#include "HttpMessage.h"
#include <string>
#include <unordered_map>

class HttpResponse : public HttpMessage {
private:
    int statusCode_;      // 状态码（如200、404等）

public:
    HttpResponse(int statusCode = 200);
    ~HttpResponse();

    // Getter方法
    int getStatusCode() const;

    // Setter方法
    void setStatusCode(int statusCode);

    // 其他方法
    void setBody(const std::string& body);
    std::string toString() const override;
};