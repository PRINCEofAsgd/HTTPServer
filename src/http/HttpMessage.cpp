#include "../../include/http/HttpMessage.h"

HttpMessage::HttpMessage() : version_("HTTP/1.1"), 
use_sendfile_(false), file_fd_(-1), file_offset_(0), file_size_(0) {}
HttpMessage::~HttpMessage() {}

const std::string& HttpMessage::getVersion() const { return version_; }
const std::unordered_map<std::string, std::string>& HttpMessage::getHeaders() const { return headers_; }
const std::string& HttpMessage::getBody() const { return body_; }
bool HttpMessage::useSendfile() const { return use_sendfile_; }
int HttpMessage::getFileFd() const { return file_fd_; }
off_t HttpMessage::getFileOffset() const { return file_offset_; }
size_t HttpMessage::getFileSize() const { return file_size_; }

void HttpMessage::setVersion(const std::string& version) { version_ = version; }
void HttpMessage::addHeader(const std::string& key, const std::string& value) { headers_[key] = value; }
void HttpMessage::setBody(const std::string& body) { body_ = body; }
void HttpMessage::setUseSendfile(bool use_sendfile) { use_sendfile_ = use_sendfile; }
void HttpMessage::setFileFd(int file_fd) { file_fd_ = file_fd; }
void HttpMessage::setFileOffset(off_t file_offset) { file_offset_ = file_offset; }
void HttpMessage::setFileSize(size_t file_size) { file_size_ = file_size; }

// 获取Connection头的值
std::string HttpMessage::getConnectionHeader() const {
    auto it = headers_.find("Connection");
    if (it != headers_.end()) return it->second;
    return "";
}

// 获取文件名头的值
std::string HttpMessage::getFilenameHeader() const {
    auto it = headers_.find("Content-Disposition");
    if (it != headers_.end()) {
        const std::string& contentDisposition = it->second;
        size_t filenamePos = contentDisposition.find("filename=");
        if (filenamePos != std::string::npos) {
            size_t start = filenamePos + 9; // "filename=" 的长度
            size_t end = contentDisposition.find(";", start);
            if (end == std::string::npos) end = contentDisposition.size();
            // 去除引号
            std::string filename = contentDisposition.substr(start, end - start);
            if (!filename.empty() && (filename[0] == '"' || filename[0] == '\'')) 
                filename = filename.substr(1, filename.size() - 2);
            return filename;
        }
    }
    return "";
}

// 获取Content-Type头的值
std::string HttpMessage::getContentType() const {
    auto it = headers_.find("Content-Type");
    if (it != headers_.end()) {
        std::string contentType = it->second;
        // 提取主类型(去除 charset 等参数)
        size_t semicolonPos = contentType.find(';');
        if (semicolonPos != std::string::npos) 
            contentType = contentType.substr(0, semicolonPos);
        // 去除首尾空格
        contentType.erase(0, contentType.find_first_not_of(' '));
        contentType.erase(contentType.find_last_not_of(' ') + 1);
        return contentType;
    }
    return "";
}

// 获取Content-Disposition头的值
std::string HttpMessage::getContentDisposition() const {
    auto it = headers_.find("Content-Disposition");
    if (it != headers_.end()) {
        std::string contentDisposition = it->second;
        // 提取主类型(去除 filename 等参数)
        size_t semicolonPos = contentDisposition.find(';');
        if (semicolonPos != std::string::npos) 
            contentDisposition = contentDisposition.substr(0, semicolonPos);
        // 去除首尾空格
        contentDisposition.erase(0, contentDisposition.find_first_not_of(' '));
        contentDisposition.erase(contentDisposition.find_last_not_of(' ') + 1);
        return contentDisposition;
    }
    return "";
}

// 解析JSON内容并返回nlohmann::json对象
nlohmann::json HttpMessage::getJson() const {
    // 使用nlohmann::json库解析JSON字符串
    nlohmann::json jsonObj = nlohmann::json::parse(body_);
    return jsonObj;
}

// 从请求头中提取Token
std::string HttpMessage::getToken() const {
    // 从请求头中获取 Authorization 字段
    auto it = headers_.find("Authorization");
    if (it == headers_.end()) return "";
    std::string authHeader = it->second;
    // 检查认证方式是否为 "Bearer {Token}"
    if (authHeader.substr(0, 7) != "Bearer ") return "";
    // 提取Token
    return authHeader.substr(7);
}