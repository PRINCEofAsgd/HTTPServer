#pragma once
#include <string>
#include "../redis/RedisClient.h"

class UserManager {
private:
    RedisClient* redis_; // Redis客户端指针
    std::string userFile; // 用户信息文件路径

public:
    UserManager(const std::string& userFile);
    ~UserManager();

    bool loadUsers(); // 从磁盘文件 userFile 加载用户信息到 Redis，服务器启动时调用一次
    bool addUser(const std::string& username, const std::string& password);     // 添加用户信息到 Redis
    std::string getPassword(const std::string& username);                       // 获取用户密码
    bool userExists(const std::string& username);                               // 检查用户是否存在，注册用
    bool verifyUser(const std::string& username, const std::string& password);  // 验证用户信息，登录用
};
