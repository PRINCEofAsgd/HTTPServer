// JSON响应处理器实现
#include "../../include/client/JsonHandler.h"
#include "../../include/client/Utils.h"
#include <stdio.h>

// 外部全局变量声明
extern ClientState state;
extern std::string token;

void JsonHandler::handle(const HttpResponse& response) {
    printf("\nJSON响应:\n");

    try {
        json jsonData = response.getJson();
        printf("%s\n", jsonData.dump(2).c_str());
        int statusCode = response.getStatusCode(); // 检查状态码
        
        if (statusCode == 401) { // 处理 401 Unauthorized错误
            printf("Token无效，请重新登录\n");
            token = "";
            state = AUTH_STATE; // 切换到认证状态
            return;
        }
        
        if (statusCode == 200) { // 处理成功的响应
            if (jsonData.contains("token") && jsonData["token"].is_string()) { // 包含 token，是登录成功的响应
                token = jsonData["token"];
                printf("登录成功！Token: %s\n", token.c_str());
                state = BUSINESS_STATE; // 切换到业务状态
            }
            else if (jsonData.contains("message") && jsonData["message"].is_string()) { // 包含 message，是注册成功的响应
                std::string message = jsonData["message"];
                if (message.find("注册成功") != std::string::npos) printf("注册成功！\n");
                else printf("成功响应: %s\n", message.c_str());
            }
        }
        else if (statusCode == 400 || statusCode == 403) printf("登录失败，请检查用户名和密码\n");
    } catch (const json::parse_error& e) {
        printf("JSON解析错误: %s\n", e.what());
    }
}