#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "UserManager.h"
#include "TokenManager.h"

class LoginController {
private:
    UserManager* userManager_;
    TokenManager* tokenManager_;

public:
    LoginController(UserManager* userManager, TokenManager* tokenManager);
    ~LoginController();

    // 处理登录请求
    HttpResponse handleLogin(const HttpRequest& request);
};