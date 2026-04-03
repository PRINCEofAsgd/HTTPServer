#include "../../include/http/StaticFileController.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <fcntl.h>

StaticFileController::StaticFileController() {}

StaticFileController::~StaticFileController() {}

// 验证路径，防止路径遍历攻击
bool StaticFileController::validatePath(const std::string& path) {
    // 检查是否包含 ".." 或 "/../" 或 "/.." 或 "../" 等路径遍历字符
    if (path.find("../") != std::string::npos) return false;
    if (path.find("..") != std::string::npos) return false;
    if (path.find("/..") != std::string::npos) return false;
    if (path.find("..") == 0) return false;
    return true;
}

HttpResponse StaticFileController::handleStaticFile(const HttpRequest& request) {
    // 获取查询参数
    std::unordered_map<std::string, std::string> params = request.getQueryParameters();
    
    // 构建文件路径
    std::string basePath = "../static";
    std::string dir = "";
    std::string fileName = "index.html";
    
    // 处理查询参数
    if (params.find("dir") != params.end()) {
        dir = params["dir"];
        // 验证路径，防止路径遍历攻击
        if (!validatePath(dir)) {
            HttpResponse response(403);
            response.addHeader("Content-Type", "text/html");
            response.setBody("<html><body><h1>403 Forbidden</h1></body></html>");
            return response;
        }
    }
    
    if (params.find("filename") != params.end()) {
        fileName = params["filename"];
        // 验证文件名，防止路径遍历攻击
        if (!validatePath(fileName)) {
            HttpResponse response(403);
            response.addHeader("Content-Type", "text/html");
            response.setBody("<html><body><h1>403 Forbidden</h1></body></html>");
            return response;
        }
    }
    
    // 构建完整路径
    std::string fullPath = basePath;
    if (!dir.empty()) {
        fullPath += "/" + dir;
    }
    fullPath += "/" + fileName;
    
    // 检查文件是否存在
    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) != 0) {
        HttpResponse response(404);
        response.addHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>404 Not Found</h1></body></html>");
        return response;
    }
    
    // 检查是否是目录
    if (S_ISDIR(fileStat.st_mode)) {
        // 如果是目录，尝试返回index.html
        fullPath += "/index.html";
        if (stat(fullPath.c_str(), &fileStat) != 0) {
            HttpResponse response(404);
            response.addHeader("Content-Type", "text/html");
            response.setBody("<html><body><h1>404 Not Found</h1></body></html>");
            return response;
        }
    }
    
    // 打开文件
    int fd = open(fullPath.c_str(), O_RDONLY);
    if (fd < 0) {
        HttpResponse response(500);
        response.addHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
        return response;
    }
    
    // 确定文件的MIME类型
    std::string contentType = "application/octet-stream";
    size_t dotPos = fileName.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string extension = fileName.substr(dotPos + 1);
        if (extension == "html" || extension == "htm") contentType = "text/html";
        else if (extension == "css") contentType = "text/css";
        else if (extension == "js") contentType = "application/javascript";
        else if (extension == "json") contentType = "application/json";
        else if (extension == "xml") contentType = "application/xml";
        else if (extension == "png") contentType = "image/png";
        else if (extension == "jpg" || extension == "jpeg") contentType = "image/jpeg";
        else if (extension == "gif") contentType = "image/gif";
        else if (extension == "ico") contentType = "image/x-icon";
        else if (extension == "txt") contentType = "text/plain";
    }
    
    // 构造响应
    HttpResponse response(200);
    response.addHeader("Content-Type", contentType);
    response.addHeader("Content-Length", std::to_string(fileStat.st_size));
    response.addHeader("Content-Disposition", "inline");
    
    // 设置文件传输相关参数
    response.setUseSendfile(true);
    response.setFileFd(fd);
    response.setFileOffset(0);
    response.setFileSize(fileStat.st_size);
    
    return response;
}