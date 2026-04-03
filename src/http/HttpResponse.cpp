#include "../../include/http/HttpResponse.h"
#include <sstream>

HttpResponse::HttpResponse(int statusCode) : HttpMessage(), statusCode_(statusCode) {}
HttpResponse::~HttpResponse() {}

int HttpResponse::getStatusCode() const { return statusCode_; }
void HttpResponse::setStatusCode(int statusCode) { statusCode_ = statusCode; }

// 设置响应体
void HttpResponse::setBody(const std::string& body) {
    HttpMessage::setBody(body);
    // 自动设置Content-Length头
    std::stringstream ss;
    ss << body.length();
    addHeader("Content-Length", ss.str());
}

// 生成完整响应报文
std::string HttpResponse::toString() const {
    std::stringstream ss;
    ss << getVersion() << " " << statusCode_; // 添加 HTTP 版本和状态码
    
    switch (statusCode_) { // 根据状态码添加状态描述
        case 200: 
            ss << " OK";
            break;
        case 400: 
            ss << " Bad Request";
            break;
        case 404: 
            ss << " Not Found";
            break;
        case 500: 
            ss << " Internal Server Error";
            break;
        default: 
            ss << "";
            break;
    }

    ss << "\r\n";   // 添加空行
    bool hasConnectionHeader = false; // 检查是否已经设置了 Connection 头，如果没有，默认添加 keep-alive
    for (const auto& header : getHeaders()) {
        if (header.first == "Connection") hasConnectionHeader = true;
        ss << header.first << ": " << header.second << "\r\n"; // 添加响应头
    }
    if (!hasConnectionHeader) {
        ss << "Connection: keep-alive\r\n";
        ss << "Keep-Alive: timeout=60, max=100\r\n";
    }
    ss << "\r\n";   // 添加空行
    ss << getBody();    // 添加响应体
    return ss.str();
}