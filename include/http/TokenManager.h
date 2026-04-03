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

    std::string generateToken(const std::string& username); // 生成32位随机Token并存储到Redis
    bool verifyToken(const std::string& token, std::string& username); // 验证Token并获取用户名
    bool deleteToken(const std::string& token); // 删除指定Token
};