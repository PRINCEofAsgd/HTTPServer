#pragma once
#include "HttpMessage.h"
#include <string>
#include <unordered_map>

class HttpRequest : public HttpMessage {
private:
    // 以下是请求行的各部分信息(按序)
    std::string method_;  // HTTP请求方法（如GET、POST等）
    std::string path_;    // 请求的资源路径
    std::unordered_map<std::string, std::string> queryParameters_; // URL中的查询参数

public:
    HttpRequest();
    ~HttpRequest();

    // Getter方法
    const std::string& getMethod() const;
    const std::string& getPath() const;
    const std::unordered_map<std::string, std::string>& getQueryParameters() const;

    // Setter方法
    void setMethod(const std::string& method);
    void setPath(const std::string& path);
    void addQueryParameter(const std::string& key, const std::string& value);

    // 格式化输出方法
    std::string toString() const override;
};