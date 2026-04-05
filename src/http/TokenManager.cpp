#include "../../include/http/TokenManager.h"
#include <random>
#include <algorithm>

TokenManager::TokenManager(RedisClient* redis) : redis_(redis), timeout_(3600) {} // 默认1小时过期
TokenManager::~TokenManager() {}

std::string TokenManager::generateToken(const std::string& username) {
    const std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const int tokenLength = 32;
    
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(0, charset.size() - 1);
    
    std::string token;
    token.reserve(tokenLength);
    for (int i = 0; i < tokenLength; ++i) token += charset[distribution(generator)];
    
    redis_->setex(token, timeout_, username); // 将 token -> username 存储到 Redis
    redis_->setex(username, timeout_, token); // 将 username -> token 存储到 Redis
    
    return token;
}

bool TokenManager::verifyToken(const std::string& token, std::string& username) {
    std::string user = redis_->get(token);
    if (user.empty()) return false;
    username = user;
    return true;
}

bool TokenManager::verifyUsername(const std::string& username, std::string& token) {
    std::string t = redis_->get(username);
    if (t.empty()) return false;
    token = t;
    return true;
}

bool TokenManager::deleteToken(const std::string& token) {
    std::string username; // 先获取 token 对应的用户名
    if (verifyToken(token, username)) redis_->del(username); // 删除 username->token 的映射
    return redis_->del(token); // 删除 token->username 的映射
}
