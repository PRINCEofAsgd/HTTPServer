#include "../../include/http/HttpContext.h"
#include <sstream>
#include <algorithm>

HttpContext::HttpContext(bool isRequest) : receivedBytes_(0), isRequest_(isRequest) {
    parseState_ = isRequest_ ? STATE_REQUEST_LINE : STATE_STATUS_LINE;
}
HttpContext::~HttpContext() {}

// 核心解析函数，根据传入对象类型自动区分调用parseRequest或parseResponse
bool HttpContext::parse(Buffer& buffer) {
    if (isRequest_) return parseRequest(buffer);
    else return parseResponse(buffer);
}

// 解析请求
bool HttpContext::parseRequest(Buffer& buffer) {
    receivedBytes_ += buffer.size();
    
    while (buffer.size() > 0) {
        switch (parseState_) {
            case STATE_REQUEST_LINE: {
                // 查找第一个\r\n，即请求行结束位置
                const char* data = buffer.data();
                size_t size = buffer.size();
                
                for (size_t i = 0; i < size - 1; ++i) {
                    if (data[i] == '\r' && data[i + 1] == '\n') {
                        // 提取请求行
                        std::string line(data, i);
                        if (!parseRequestLine(line)) return false;
                        
                        // 移除已解析的请求行
                        buffer.erase(0, i + 2);
                        parseState_ = STATE_HEADERS;
                        break;
                    }
                }
                if (parseState_ != STATE_HEADERS) return false; // 没有找到完整的请求行
                break;
            }
            
            case STATE_STATUS_LINE:
                // 解析请求时不应该进入状态行状态
                return false;
                
            case STATE_HEADERS: {
                const char* data = buffer.data();
                size_t size = buffer.size();
                
                // 查找\r\n\r\n，即请求头结束位置
                for (size_t i = 0; i < size - 3; ++i) {
                    if (data[i] == '\r' && data[i + 1] == '\n' && data[i + 2] == '\r' && data[i + 3] == '\n') {
                        // 提取请求头
                        std::string headersStr(data, i);
                        std::istringstream iss(headersStr);
                        std::vector<std::string> headerLines;
                        std::string line;
                        
                        while (std::getline(iss, line)) 
                            if (!line.empty()) headerLines.push_back(line);
                        
                        if (!parseHeaders(headerLines)) return false;
                        // 移除已解析的请求头
                        buffer.erase(0, i + 4);
                        
                        // 检查是否有请求体
                        auto it = request_.getHeaders().find("Content-Length");
                        if (it != request_.getHeaders().end()) {
                            size_t contentLength = std::stoul(it->second);
                            if (contentLength > 0) parseState_ = STATE_BODY; 
                            else parseState_ = STATE_COMPLETE;
                        } 
                        else parseState_ = STATE_COMPLETE; // 没有Content-Length头，假设没有请求体
                        break;
                    }
                }
                if (parseState_ != STATE_BODY && parseState_ != STATE_COMPLETE) return false; // 没有找到完整的请求头
                break;
            }
            
            case STATE_BODY: {
                auto it = request_.getHeaders().find("Content-Length");
                if (it != request_.getHeaders().end()) {
                    size_t contentLength = std::stoul(it->second);
                    if (buffer.size() >= contentLength) {
                        // 提取请求体
                        std::string body(buffer.data(), contentLength);
                        request_.setBody(body);
                        buffer.erase(0, contentLength);
                        parseState_ = STATE_COMPLETE;
                    } 
                    else return false;              // 没有足够的请求体数据
                } 
                else parseState_ = STATE_COMPLETE;  // 没有Content-Length头，假设没有请求体
                break;
            }
            
            case STATE_COMPLETE:
                return true;
        }
    }
    
    return isComplete();
}

// 解析响应
bool HttpContext::parseResponse(Buffer& buffer) {
    receivedBytes_ += buffer.size();
    
    while (buffer.size() > 0) {
        switch (parseState_) {
            case STATE_REQUEST_LINE:
                // 解析响应时不应该进入请求行状态
                return false;
                
            case STATE_STATUS_LINE: {
                // 查找第一个\r\n，即状态行结束位置
                const char* data = buffer.data();
                size_t size = buffer.size();
                
                for (size_t i = 0; i < size - 1; ++i) {
                    if (data[i] == '\r' && data[i + 1] == '\n') {
                        // 提取状态行
                        std::string line(data, i);
                        if (!parseStatusLine(line)) return false;
                        
                        // 移除已解析的状态行
                        buffer.erase(0, i + 2);
                        parseState_ = STATE_HEADERS;
                        break;
                    }
                }
                if (parseState_ != STATE_HEADERS) return false; // 没有找到完整的状态行
                break;
            }
            
            case STATE_HEADERS: {
                const char* data = buffer.data();
                size_t size = buffer.size();
                
                // 查找\r\n\r\n，即响应头结束位置
                for (size_t i = 0; i < size - 3; ++i) {
                    if (data[i] == '\r' && data[i + 1] == '\n' && data[i + 2] == '\r' && data[i + 3] == '\n') {
                        // 提取响应头
                        std::string headersStr(data, i);
                        std::istringstream iss(headersStr);
                        std::vector<std::string> headerLines;
                        std::string line;
                        
                        while (std::getline(iss, line)) 
                            if (!line.empty()) headerLines.push_back(line);
                        
                        if (!parseHeaders(headerLines)) return false;
                        // 移除已解析的响应头
                        buffer.erase(0, i + 4);
                        
                        // 检查是否有响应体
                        auto it = response_.getHeaders().find("Content-Length");
                        if (it != response_.getHeaders().end()) {
                            size_t contentLength = std::stoul(it->second);
                            if (contentLength > 0) parseState_ = STATE_BODY; 
                            else parseState_ = STATE_COMPLETE;
                        } 
                        else parseState_ = STATE_COMPLETE; // 没有Content-Length头，假设没有响应体
                        break;
                    }
                }
                if (parseState_ != STATE_BODY && parseState_ != STATE_COMPLETE) return false; // 没有找到完整的响应头
                break;
            }
            
            case STATE_BODY: {
                auto it = response_.getHeaders().find("Content-Length");
                if (it != response_.getHeaders().end()) {
                    size_t contentLength = std::stoul(it->second);
                    if (buffer.size() >= contentLength) {
                        // 提取响应体
                        std::string body(buffer.data(), contentLength);
                        response_.setBody(body);
                        buffer.erase(0, contentLength);
                        parseState_ = STATE_COMPLETE;
                    } 
                    else return false;              // 没有足够的响应体数据
                } 
                else parseState_ = STATE_COMPLETE;  // 没有Content-Length头，假设没有响应体
                break;
            }
            
            case STATE_COMPLETE:
                return true;
        }
    }
    
    return isComplete();
}

// 解析请求行
bool HttpContext::parseRequestLine(const std::string& line) {
    std::istringstream iss(line);
    std::string method, path, version;
    if (!(iss >> method >> path >> version)) return false;
    
    // 解析路径和查询参数
    size_t queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        std::string pathPart = path.substr(0, queryPos);
        std::string queryPart = path.substr(queryPos + 1);
        
        request_.setPath(pathPart);
        
        // 解析查询参数
        size_t pos = 0;
        while (pos < queryPart.size()) {
            size_t ampPos = queryPart.find('&', pos);
            if (ampPos == std::string::npos) ampPos = queryPart.size();
            
            std::string param = queryPart.substr(pos, ampPos - pos);
            size_t eqPos = param.find('=');
            if (eqPos != std::string::npos) {
                std::string key = param.substr(0, eqPos);
                std::string value = param.substr(eqPos + 1);
                request_.addQueryParameter(key, value);
            }
            
            pos = ampPos + 1;
        }
    } 
    else request_.setPath(path);
    
    request_.setMethod(method);
    request_.setVersion(version);
    
    return true;
}

// 解析状态行
bool HttpContext::parseStatusLine(const std::string& line) {
    std::istringstream iss(line);
    std::string version, statusCodeStr, statusMessage;
    if (!(iss >> version >> statusCodeStr)) return false;
    
    // 解析状态码
    int statusCode = std::stoi(statusCodeStr);
    
    // 读取状态消息（可能包含空格）
    std::getline(iss, statusMessage);
    if (!statusMessage.empty() && statusMessage[0] == ' ') 
        statusMessage = statusMessage.substr(1);
    
    response_.setVersion(version);
    response_.setStatusCode(statusCode);
    
    return true;
}

bool HttpContext::parseHeaders(const std::vector<std::string>& lines) {
    for (const auto& line : lines) {
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) return false;
        
        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        
        // 去除value前后的空格和回车符
        value.erase(0, value.find_first_not_of(" \r"));
        value.erase(value.find_last_not_of(" \r") + 1);
        
        if (isRequest_) request_.addHeader(key, value);
        else response_.addHeader(key, value);
    }
    return true;
}

bool HttpContext::parseBody(const Buffer& buffer, size_t contentLength) {
    if (buffer.size() >= contentLength) {
        std::string body(buffer.data(), contentLength);
        if (isRequest_) request_.setBody(body);
        else response_.setBody(body);
        return true;
    }
    return false;
}

const HttpRequest& HttpContext::getRequest() const { return request_; }
const HttpResponse& HttpContext::getResponse() const { return response_; }

bool HttpContext::isComplete() const { return parseState_ == STATE_COMPLETE; }

void HttpContext::reset() {
    request_ = HttpRequest();
    response_ = HttpResponse();
    parseState_ = isRequest_ ? STATE_REQUEST_LINE : STATE_STATUS_LINE;
    tempBuffer_.clear();
    receivedBytes_ = 0;
}