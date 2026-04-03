#include "../../include/http/AuthorMiddleWare.h"
#include "../../third_party/nlohmann/json.hpp"
#include <string>
#include <sstream>

using json = nlohmann::json;

AuthorMiddleWare::AuthorMiddleWare(TokenManager* tokenManager) : tokenManager_(tokenManager) {}
AuthorMiddleWare::~AuthorMiddleWare() {}

bool AuthorMiddleWare::verifyToken(const HttpRequest& request, std::string& username) {
    std::string token = request.getToken();                         // 从请求中提取Token
    if (token.empty()) return false;                                // 未返回正确的 Token
    if (!tokenManager_->verifyToken(token, username)) return false; // Token 无效
    return true; // Token 有效
}

bool AuthorMiddleWare::verifyToken(const HttpRequest& request) {
    std::string username;
    return verifyToken(request, username);
}