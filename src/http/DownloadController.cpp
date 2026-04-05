#include "../../include/http/DownloadController.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <fcntl.h>
#include "../../third_party/nlohmann/json.hpp"

using json = nlohmann::json;

DownloadController::DownloadController(AuthorMiddleWare* authorMiddleWare) : authorMiddleWare_(authorMiddleWare) {}
DownloadController::~DownloadController() {}

// 验证路径，防止路径遍历攻击
bool DownloadController::validatePath(const std::string& path) {
    // 检查是否包含 ".." 或 "/../" 或 "/.." 或 "../" 等路径遍历字符
    if (path.find("../") != std::string::npos) return false;
    if (path.find("..") != std::string::npos) return false;
    if (path.find("/..") != std::string::npos) return false;
    if (path.find("..") == 0) return false;
    return true;
}

HttpResponse DownloadController::handleFileDownload(const HttpRequest& request) {
    // 验证Token并获取用户名
    std::string username;
    if (!authorMiddleWare_->verifyToken(request, username)) {
        HttpResponse response(401);
        response.addHeader("Content-Type", "application/json");
        json respBody;
        respBody["message"] = "Invalid or expired token";
        response.setBody(respBody.dump());
        return response;
    }

    // 获取查询参数
    std::unordered_map<std::string, std::string> params = request.getQueryParameters();
    
    // 构建文件路径
    std::string basePath = "/home/loki/桌面/HttpStaticFiles";
    std::string fileName = "";
    
    // 处理查询参数
    if (params.find("filename") != params.end()) {
        fileName = params["filename"];
        if (!validatePath(fileName)) { // 验证文件名，防止路径遍历攻击
            HttpResponse response(403);
            response.addHeader("Content-Type", "text/html");
            response.setBody("<html><body><h1>403 Forbidden</h1></body></html>");
            return response;
        }
    } 
    else {
        HttpResponse response(400);
        response.addHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>400 Bad Request</h1><p>缺少文件名</p></body></html>");
        return response;
    }
    
    // 构建完整路径
    std::string fullPath = basePath + "/" + username + "/" + fileName;
    
    // 检查用户目录是否存在
    struct stat userDirStat;
    std::string userDir = basePath + "/" + username;
    if (stat(userDir.c_str(), &userDirStat) != 0 || !S_ISDIR(userDirStat.st_mode)) {
        HttpResponse response(404);
        response.addHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>404 Not Found</h1><p>用户不存在</p></body></html>");
        return response;
    }
    
    // 检查文件是否存在
    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) != 0) {
        HttpResponse response(404);
        response.addHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>404 Not Found</h1><p>文件不存在</p></body></html>");
        return response;
    }
    
    // 检查是否是目录
    if (S_ISDIR(fileStat.st_mode)) {
        HttpResponse response(400);
        response.addHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>400 Bad Request</h1><p>不能下载目录</p></body></html>");
        return response;
    }
    
    // 打开文件
    int fd = open(fullPath.c_str(), O_RDONLY);
    if (fd < 0) {
        HttpResponse response(500);
        response.addHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
        return response;
    }
    
    // 确定文件的 MIME 类型，根据文件扩展名判断，用于构造响应头中的 Content-Type 字段
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
    response.addHeader("Content-Disposition", "attachment; filename=\"" + fileName + "\"");
    
    // 设置文件传输相关参数
    response.setUseSendfile(true);
    response.setFileFd(fd);
    response.setFileOffset(0);
    response.setFileSize(fileStat.st_size);
    
    return response;
}