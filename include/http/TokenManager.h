#pragma once
#include <string>
#include "../redis/RedisClient.h"

class TokenManager {
private:
    RedisClient* redis_; // Redis客户端指针
    int timeout_; // token过期时间（秒），默认1小时

public:
    TokenManager(RedisClient* redis);
    ~TokenManager();

    std::string generateToken(const std::string& username);                 // 生成 32 位随机 Token 并存储到 Redis
    bool verifyToken(const std::string& token, std::string& username);      // 验证 Token 并获取用户名
    bool verifyUsername(const std::string& username, std::string& token);   // 验证用户名并获取 Token
    bool deleteToken(const std::string& token);                             // 删除指定 Token 
};