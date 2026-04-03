#pragma once
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"
#include "TokenManager.h"

class AuthorMiddleWare {
private:
    TokenManager* tokenManager_;

public:
    AuthorMiddleWare(TokenManager* tokenManager);
    ~AuthorMiddleWare();

    bool verifyToken(const HttpRequest& request, std::string& username); // 验证 Token 是否有效并获取用户名
    bool verifyToken(const HttpRequest& request); // 验证 Token 是否有效(可能错误或过期)
};