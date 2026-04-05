#include "../../include/http/RegisterController.h"
#include "../../third_party/nlohmann/json.hpp"
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <sstream>
#include <iostream>

using json = nlohmann::json;

RegisterController::RegisterController(UserManager* userManager, const std::string& staticFilesPath) 
    : userManager_(userManager), staticFilesPath_(staticFilesPath) {}
RegisterController::~RegisterController() {}

// 创建目录
bool RegisterController::createDirectory(const std::string& dirPath) {
    struct stat statBuf;
    if (stat(dirPath.c_str(), &statBuf) != 0) {
        // 目录不存在，创建它
        if (mkdir(dirPath.c_str(), 0755) != 0) {
            // 如果创建失败，可能是因为父目录不存在，尝试递归创建
            size_t pos = dirPath.find_last_of('/');
            if (pos != std::string::npos) {
                std::string parentDir = dirPath.substr(0, pos);
                // 递归创建父目录
                if (!createDirectory(parentDir)) return false;
                // 再次尝试创建目录
                if (mkdir(dirPath.c_str(), 0755) != 0) {
                    std::cerr << "创建目录失败: " << dirPath << ", 错误: " << strerror(errno) << std::endl;
                    return false;
                }
            } 
            else return false;
        }
    } 
    // 目录存在，检查是否为目录
    else if (!S_ISDIR(statBuf.st_mode)) { // 如果不是目录
        std::cerr << "路径已存在但不是目录: " << dirPath << std::endl;
        return false;
    }
    return true;
}

HttpResponse RegisterController::handleRegister(const HttpRequest& request) {
    // 检查Content-Type
    auto headers = request.getHeaders();
    auto it = headers.find("Content-Type");
    if (it == headers.end() || it->second != "application/json") {
        HttpResponse response(400);
        response.addHeader("Content-Type", "application/json");
        response.setBody(R"({"error": "Content-Type must be application/json"})");
        return response;
    }

    // 解析请求体
    std::string body = request.getBody();
    json requestData;
    try {
        requestData = json::parse(body);
    } catch (...) {
        HttpResponse response(400);
        response.addHeader("Content-Type", "application/json");
        response.setBody(R"({"error": "Invalid JSON format"})");
        return response;
    }

    // 检查是否包含username和password字段
    if (!requestData.contains("username") || !requestData.contains("password")) {
        HttpResponse response(400);
        response.addHeader("Content-Type", "application/json");
        response.setBody(R"({"error": "Missing username or password"})");
        return response;
    }

    std::string username = requestData["username"];
    std::string password = requestData["password"];

    // 检查用户名是否已存在
    if (userManager_->userExists(username)) {
        HttpResponse response(409);
        response.addHeader("Content-Type", "application/json");
        response.setBody(R"({"error": "Username already exists"})");
        return response;
    }

    // 添加用户到 Redis
    if (!userManager_->addtempUser(username, password)) {
        HttpResponse response(500);
        response.addHeader("Content-Type", "application/json");
        response.setBody(R"({"error": "Failed to save user information to Redis"})");
        return response;
    }
    
    // 添加用户到 MySQL
    if (!userManager_->registerUser(username, password)) {
        HttpResponse response(500);
        response.addHeader("Content-Type", "application/json");
        response.setBody(R"({"error": "Failed to save user information to MySQL"})");
        return response;
    }

    // 创建用户目录
    std::string userDir = staticFilesPath_ + "/" + username;
    if (!createDirectory(userDir)) {
        // 目录创建失败，但用户信息已经保存，仍然返回成功
        std::cerr << "警告: 无法创建用户目录，但注册成功" << std::endl;
    }

    // 返回成功响应
    HttpResponse response(200);
    response.addHeader("Content-Type", "application/json");
    response.setBody(R"({"success": true, "message": "Registration successful"})");
    return response;
}
