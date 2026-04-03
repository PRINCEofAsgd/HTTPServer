#include "../../include/http/LoginController.h"
#include "../../third_party/nlohmann/json.hpp"
#include <iostream>

using json = nlohmann::json;

LoginController::LoginController(UserManager* userManager, TokenManager* tokenManager) 
    : userManager_(userManager), tokenManager_(tokenManager) {}
LoginController::~LoginController() {}

HttpResponse LoginController::handleLogin(const HttpRequest& request) {
    // 解析请求体
    std::string body = request.getBody();
    try {
        json reqBody = json::parse(body);
        
        // 提取用户名和密码
        std::string username = reqBody["username"];
        std::string password = reqBody["password"];
        
        // 验证用户信息
        if (userManager_->verifyUser(username, password)) { // 用户密码正确，响应返回 Token
            // 生成Token并保存到Redis
            std::string token = tokenManager_->generateToken(username);

            HttpResponse response(200);
            response.addHeader("Content-Type", "application/json");
            
            json respBody;
            respBody["token"] = token;
            respBody["message"] = "Login successful";
            
            response.setBody(respBody.dump());
            return response;
        } 
        else { // 用户密码错误，响应返回 401
            HttpResponse response(401);
            response.addHeader("Content-Type", "application/json");
            
            json respBody;
            respBody["message"] = "Invalid username or password";
            
            response.setBody(respBody.dump());
            return response;
        }
    } 
    catch (const json::exception& e) { // 解析失败，响应返回 400
        HttpResponse response(400);
        response.addHeader("Content-Type", "application/json");
        
        json respBody;
        respBody["message"] = "Invalid request body";
        
        response.setBody(respBody.dump());
        return response;
    }
}