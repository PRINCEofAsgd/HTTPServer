#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "../core/Buffer.h"
#include <string>
#include <vector>

class HttpContext {
private:
    enum ParseState {       // 解析状态枚举
        STATE_REQUEST_LINE, // 解析请求行(HttpRequest 对象)
        STATE_STATUS_LINE,  // 解析状态行(HttpResponse 对象)
        STATE_HEADERS,      // 解析请求头/响应头
        STATE_BODY,         // 解析请求体/响应体
        STATE_COMPLETE      // 解析完成
    };
    HttpRequest request_;           // 当前解析的 HttpRequest 对象
    HttpResponse response_;         // 当前解析的 HttpResponse 对象
    ParseState parseState_;         // 当前解析状态
    std::vector<char> tempBuffer_;  // 用于存储解析过程中的临时数据
    size_t receivedBytes_;          // 记录已接收的字节数量
    bool isRequest_;                // 标记当前解析的是请求还是响应

public:
    HttpContext(bool isRequest = true);
    ~HttpContext();

    // 核心解析函数，根据传入对象类型自动区分调用 parseRequest 或 parseResponse
    bool parse(Buffer& buffer);
    
    // 解析请求
    bool parseRequest(Buffer& buffer);
    bool parseRequestLine(const std::string& line);             // 解析请求行
    
    // 解析响应
    bool parseResponse(Buffer& buffer);
    bool parseStatusLine(const std::string& line);              // 解析状态行
    
    // 通用解析方法
    bool parseHeaders(const std::vector<std::string>& lines);   // 解析请求头/响应头
    bool parseBody(const Buffer& buffer, size_t contentLength); // 解析请求体/响应体
    
    // 获取解析结果
    const HttpRequest& getRequest() const;                      // 获取请求解析结果
    const HttpResponse& getResponse() const;                    // 获取响应解析结果
    
    bool isComplete() const;                                    // 检查是否解析完成
    void reset();                                               // 重置解析状态
};