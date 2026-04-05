#pragma once
#include <string>
#include <unordered_map>
#include "../../third_party/nlohmann/json.hpp"

class HttpMessage {
protected:
    std::string version_; // HTTP版本
    std::unordered_map<std::string, std::string> headers_; // 消息头信息
    std::string body_; // 消息体内容
    
    // 文件传输相关成员
    bool use_sendfile_; // 是否使用sendfile发送文件
    int file_fd_; // 文件描述符
    off_t file_offset_; // 文件偏移量
    size_t file_size_; // 文件大小

public:
    HttpMessage();
    virtual ~HttpMessage();

    // Getter 方法
    const std::string& getVersion() const;
    const std::unordered_map<std::string, std::string>& getHeaders() const;
    const std::string& getBody() const;
    nlohmann::json getJson() const;
    std::string getToken() const;

    // 消息体相关 Getter 方法
    std::string getConnectionHeader() const;
    std::string getFilenameHeader() const;
    std::string getContentType() const;
    std::string getContentDisposition() const;
    std::string getXRequestId() const;
    
    // 文件传输相关 Getter 方法
    bool useSendfile() const;
    int getFileFd() const;
    off_t getFileOffset() const;
    size_t getFileSize() const;

    // Setter方法
    void setVersion(const std::string& version);
    void addHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);
    
    // 文件传输相关 Setter 方法
    void setUseSendfile(bool use_sendfile);
    void setFileFd(int file_fd);
    void setFileOffset(off_t file_offset);
    void setFileSize(size_t file_size);

    // 虚方法，子类 HttpRequest 和 HttpResponse 必须实现
    virtual std::string toString() const = 0;
};