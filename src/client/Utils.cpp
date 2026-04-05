// 客户端工具函数实现
#include "../../include/client/Utils.h"
#include "../../include/client/HandlerFactory.h"
#include "../../include/http/HttpRequest.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <fstream>

// 静态文件保存目录
const std::string STATIC_FILE_DIR = "/home/loki/桌面/HttpStaticFiles/publicFiles";

// 服务器地址和端口
const std::string SERVER_IP = "192.168.1.128";
const std::string SERVER_PORT = "8079";

// 心跳检测相关常量
const int HEARTBEAT_INTERVAL = 20; // 心跳检测时间间隔（秒）

// 全局变量
ClientState state = AUTH_STATE;
std::string token = "";
int requestId_ = 1000;

void stop(int sig) {
    printf("收到信号 %d, 退出程序\n", sig);
    exit(0);
}

// 创建目录
void createDirectory(const std::string& dirPath) { // 递归创建目录
    struct stat statBuf;
    if (stat(dirPath.c_str(), &statBuf) != 0) { // 目录不存在，创建它
        if (mkdir(dirPath.c_str(), 0755) != 0) { // 如果创建失败，可能是因为父目录不存在，尝试递归创建
            size_t pos = dirPath.find_last_of('/');
            if (pos != std::string::npos) {
                std::string parentDir = dirPath.substr(0, pos);
                createDirectory(parentDir);
                // 再次尝试创建目录
                if (mkdir(dirPath.c_str(), 0755) != 0) 
                    printf("创建目录失败: %s, 错误: %s\n", dirPath.c_str(), strerror(errno));
            }
        }
    } 
    else if (!S_ISDIR(statBuf.st_mode)) printf("路径已存在但不是目录: %s\n", dirPath.c_str()); // 路径存在但不是目录
}

// 用户输入处理函数
void getFileInput(std::string& input, const std::string& prompt) {
    std::cout << "\n" << prompt << ": ";
    std::getline(std::cin, input);
}

void getUserInput(std::string& username, std::string& password) {
    std::cout << "\n请输入用户名: ";
    std::getline(std::cin, username);
    
    while (username.empty()) { // 输入验证
        std::cout << "用户名不能为空，请重新输入: ";
        std::getline(std::cin, username);
    }
    
    std::cout << "请输入密码: ";
    std::getline(std::cin, password);
    
    while (password.empty()) { // 输入验证
        std::cout << "密码不能为空，请重新输入: ";
        std::getline(std::cin, password);
    }
}

// 业务流程封装函数实现
void ProcessGetStatic(HandlerFactory& factory) {
    HttpRequest request;
    std::string fileName;
    getFileInput(fileName, "请输入静态文件名");
    
    // 自增请求ID
    requestId_++;
    
    // 构造获取静态资源请求
    request.setMethod("GET");
    request.setPath("/");
    if (!fileName.empty()) request.addQueryParameter("filename", fileName);
    request.setVersion("HTTP/1.1");
    request.addHeader("Host", SERVER_IP + ":" + SERVER_PORT);
    request.addHeader("Connection", "keep-alive");
    request.addHeader("X-Request-ID", std::to_string(requestId_));
    
    // 注册路由
    factory.RegisterRouter(requestId_, ReqType::GET_STATIC);
    
    // 使用 ClientConn 发送请求
    factory.handleRequest(request);
}

void ProcessLogin(HandlerFactory& factory) {
    HttpRequest request;
    std::string username, password;
    getUserInput(username, password); // 获取登录信息
    
    // 自增请求ID
    requestId_++;
    
    // 构造登录请求
    request.setMethod("POST");
    request.setPath("/login");
    request.setVersion("HTTP/1.1");
    request.addHeader("Host", SERVER_IP + ":" + SERVER_PORT);
    request.addHeader("Connection", "keep-alive");
    request.addHeader("Content-Type", "application/json");
    request.addHeader("X-Request-ID", std::to_string(requestId_));
    std::string jsonBody = "{\"username\":\"" + username + "\",\"password\":\"" + password + "\"}";
    request.addHeader("Content-Length", std::to_string(jsonBody.size()));
    request.setBody(jsonBody);
    
    // 注册路由
    factory.RegisterRouter(requestId_, ReqType::LOGIN);
    
    // 使用 ClientConn 发送请求
    factory.handleRequest(request);
}

void ProcessRegister(HandlerFactory& factory) {
    HttpRequest request;
    std::string username, password;
    getUserInput(username, password); // 获取注册信息
    
    // 自增请求ID
    requestId_++;
    
    // 构造注册请求
    request.setMethod("POST");
    request.setPath("/register");
    request.setVersion("HTTP/1.1");
    request.addHeader("Host", SERVER_IP + ":" + SERVER_PORT);
    request.addHeader("Connection", "keep-alive");
    request.addHeader("Content-Type", "application/json");
    request.addHeader("X-Request-ID", std::to_string(requestId_));
    std::string jsonBody = "{\"username\":\"" + username + "\",\"password\":\"" + password + "\"}";
    request.addHeader("Content-Length", std::to_string(jsonBody.size()));
    request.setBody(jsonBody);
    
    // 注册路由
    factory.RegisterRouter(requestId_, ReqType::REGISTER);
    
    // 使用 ClientConn 发送请求
    factory.handleRequest(request);
}

void ProcessDownloadFile(HandlerFactory& factory) {
    HttpRequest request;
    std::string fileName;
    getFileInput(fileName, "请输入要下载的文件名");
    
    // 自增请求ID
    requestId_++;
    
    // 构造下载个人文件请求
    request.setMethod("GET");
    request.setPath("/download");
    if (!fileName.empty()) request.addQueryParameter("filename", fileName);
    request.setVersion("HTTP/1.1");
    request.addHeader("Host", SERVER_IP + ":" + SERVER_PORT);
    request.addHeader("Connection", "keep-alive");
    request.addHeader("X-Request-ID", std::to_string(requestId_));
    if (!token.empty()) request.addHeader("Authorization", "Bearer " + token);
    
    // 注册路由
    factory.RegisterRouter(requestId_, ReqType::DOWNLOAD_FILE);
    
    // 使用 ClientConn 发送请求
    factory.handleRequest(request);
}

void ProcessUploadFile(HandlerFactory& factory) {
    HttpRequest request;
    std::string fileName;
    getFileInput(fileName, "请输入待上传的文件路径");
    
    // 打开文件
    int file_fd = open(fileName.c_str(), O_RDONLY);
    if (file_fd < 0) {
        printf("错误: 无法打开文件 %s\n", fileName.c_str());
        return;
    }
    
    // 获取文件大小
    struct stat file_stat;
    if (stat(fileName.c_str(), &file_stat) < 0) {
        printf("错误: 无法获取文件信息 %s\n", fileName.c_str());
        close(file_fd);
        return;
    }
    size_t file_size = file_stat.st_size;
    
    // 提取文件名
    size_t slashPos = fileName.find_last_of('/');
    std::string uploadFileName = fileName;
    if (slashPos != std::string::npos) uploadFileName = fileName.substr(slashPos + 1);
    
    // 自增请求ID
    requestId_++;
    
    // 构造上传文件请求
    request.setMethod("POST");
    request.setPath("/upload");
    request.setVersion("HTTP/1.1");
    request.addHeader("Host", SERVER_IP + ":" + SERVER_PORT);
    request.addHeader("Connection", "keep-alive");
    request.addHeader("Content-Type", "application/octet-stream");
    request.addHeader("Content-Length", std::to_string(file_size));
    request.addHeader("X-Filename", uploadFileName);
    request.addHeader("X-Request-ID", std::to_string(requestId_));
    if (!token.empty()) request.addHeader("Authorization", "Bearer " + token);
    
    // 显式设置文件相关参数
    request.setUseSendfile(true);
    request.setFileFd(file_fd);
    request.setFileOffset(0);
    request.setFileSize(file_size);
    
    // 注册路由
    factory.RegisterRouter(requestId_, ReqType::UPLOAD_FILE);
    
    // 使用 ClientConn 发送请求
    factory.handleRequest(request);
}
