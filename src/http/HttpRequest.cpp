#include "../../include/http/HttpRequest.h"
#include <sstream>

HttpRequest::HttpRequest() : HttpMessage() {}
HttpRequest::~HttpRequest() {}

// Getter方法
const std::string& HttpRequest::getMethod() const { return method_; }
const std::string& HttpRequest::getPath() const { return path_; }
const std::unordered_map<std::string, std::string>& HttpRequest::getQueryParameters() const { return queryParameters_; }

// Setter方法
void HttpRequest::setMethod(const std::string& method) { method_ = method; }
void HttpRequest::setPath(const std::string& path) { path_ = path; }
void HttpRequest::addQueryParameter(const std::string& key, const std::string& value) { queryParameters_[key] = value; }

// 格式化输出方法
std::string HttpRequest::toString() const {
    std::stringstream ss;
    ss << method_ << " " << path_;      // 添加请求方法、路径
    
    if (!queryParameters_.empty()) {    // 添加 URL 查询参数
        // URL 参数格式参照如下: ?webview_progress_bar=1&show_loading=0&push_animated=1&theme=light
        ss << "?";
        bool first = true;
        for (const auto& param : queryParameters_) {
            if (!first) ss << "&";
            ss << param.first << "=" << param.second;
            first = false;
        }
    }
    
    ss << " " << getVersion() << "\r\n";    // 添加 HTTP 版本
    for (const auto& header : getHeaders()) ss << header.first << ": " << header.second << "\r\n";  // 添加请求头
    ss << "\r\n";   // 添加空行
    ss << getBody();    // 添加请求体
    return ss.str();
}