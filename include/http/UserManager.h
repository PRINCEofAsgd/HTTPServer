#pragma once
#include <string>
#include "../redis/RedisClient.h"
#include "../mysql/MySQLClient.h"

class UserManager {
private:
    RedisClient* redis_;        // Redis客户端指针
    MySQLClient* mysqlClient_;  // MySQL客户端指针

public:
    UserManager(RedisClient* redis, MySQLClient* mysql);
    ~UserManager();

    bool addtempUser(const std::string& username, const std::string& password);     // 添加用户信息到 Redis
    bool registerUser(const std::string& username, const std::string& password);    // 添加用户信息到 MySQL
    bool verifyUser(const std::string& username, const std::string& password);      // 验证用户信息，登录用
    bool userExists(const std::string& username);           // 检查用户是否存在，注册用
    std::string getPassword(const std::string& username);   // 获取用户密码
};
