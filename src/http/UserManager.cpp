#include "../../include/http/UserManager.h"

UserManager::UserManager(RedisClient* redis, MySQLClient* mysql) : redis_(redis), mysqlClient_(mysql) {
    printf("UserManager initialized with Redis and MySQL\n");
}
UserManager::~UserManager() {}

bool UserManager::addtempUser(const std::string& username, const std::string& password) {
    if (!redis_->get("user:" + username).empty()) return false;
    return redis_->set("user:" + username, password);
}

bool UserManager::registerUser(const std::string& username, const std::string& password) {
    std::string sql = "INSERT INTO users (username, password) VALUES ('" + username + "', '" + password + "')";
    return mysqlClient_->execute(sql);
}

bool UserManager::userExists(const std::string& username) {
    // 先查 Redis
    if (!redis_->get("user:" + username).empty()) return true;
    
    // 再查 MySQL
    std::string sql = "SELECT 1 FROM users WHERE username = '" + username + "'";
    auto result = mysqlClient_->query(sql);
    return !result.empty();
}

bool UserManager::verifyUser(const std::string& username, const std::string& password) {
    // 先查 Redis
    std::string storedPassword = redis_->get("user:" + username);
    if (!storedPassword.empty()) return storedPassword == password;
    
    // 再查 MySQL
    std::string sql = "SELECT password FROM users WHERE username = '" + username + "'";
    auto result = mysqlClient_->query(sql);
    if (!result.empty()) {
        storedPassword = result[0]["password"];
        // 将结果写入 Redis 缓存
        redis_->set("user:" + username, storedPassword);
        return storedPassword == password;
    }
    return false;
}

std::string UserManager::getPassword(const std::string& username) {
    // 先查 Redis
    std::string password = redis_->get("user:" + username);
    if (!password.empty()) return password;
    
    // 再查 MySQL
    std::string sql = "SELECT password FROM users WHERE username = '" + username + "'";
    auto result = mysqlClient_->query(sql);
    if (!result.empty()) {
        password = result[0]["password"];
        // 将结果写入 Redis 缓存
        redis_->set("user:" + username, password);
    }
    return password;
}
