#include "../../include/http/UploadController.h"
#include <fstream>
#include <sys/stat.h>

UploadController::UploadController() {}

UploadController::~UploadController() {}

HttpResponse UploadController::handleFileUpload(const HttpRequest& request, const std::string& username) {
    // 确定文件存储路径
    std::string basePath = "/home/loki/桌面/HttpStaticFiles";
    std::string uploadPath;
    if (!username.empty()) uploadPath = basePath + "/" + username; 
    else uploadPath = basePath + "/publicFiles";
    
    // 检查目录是否存在
    struct stat statBuf;
    if (stat(uploadPath.c_str(), &statBuf) != 0) { // 目录不存在
        if (!username.empty()) {
            HttpResponse response(404);
            response.addHeader("Content-Type", "text/html");
            response.setBody("<html><body><h1>404 Not Found</h1><p>用户不存在</p></body></html>");
            return response;
        } 
        else {
            HttpResponse response(500);
            response.addHeader("Content-Type", "text/html");
            response.setBody("<html><body><h1>500 Internal Server Error</h1><p>公共目录不存在</p></body></html>");
            return response;
        }
    } 
    else if (!S_ISDIR(statBuf.st_mode)) { // 路径存在但不是目录
        HttpResponse response(500);
        response.addHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>500 Internal Server Error</h1><p>路径存在但不是目录</p></body></html>");
        return response;
    }
    
    // 提取文件名
    std::string fileName = "";
    auto headers = request.getHeaders();
    auto it = headers.find("X-Filename");
    if (it != headers.end()) { fileName = it->second; }
    if (fileName.empty()) {
        HttpResponse response(400);
        response.addHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>400 Bad Request</h1><p>缺少文件名</p></body></html>");
        return response;
    }
    
    std::string filePath = uploadPath + "/" + fileName; // 构建完整的文件路径
    
    // 写入文件
    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile) {
        HttpResponse response(500);
        response.addHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>500 Internal Server Error</h1><p>无法创建文件</p></body></html>");
        return response;
    }
    
    // 写入文件内容
    outFile << request.getBody();
    if (!outFile) {
        HttpResponse response(500);
        response.addHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>500 Internal Server Error</h1><p>文件写入失败</p></body></html>");
        return response;
    }
    outFile.close();
    
    // 构造响应
    HttpResponse response(200);
    response.addHeader("Content-Type", "text/html");
    response.addHeader("Content-Disposition", "inline");
    response.setBody("<html><body><h1>200 OK</h1><p>文件上传成功</p></body></html>");
    return response;
}