#include "../../include/http/UserManager.h"
#include <fstream>
#include "../../third_party/nlohmann/json.hpp"

using json = nlohmann::json;

UserManager::UserManager(const std::string& userFile) : redis_(new RedisClient()), userFile(userFile) {
    printf("UserManager initialized with user file: %s\n", userFile.c_str());
    redis_->connect();
    if (loadUsers()) printf("Users loaded successfully\n");
    else printf("Failed to load users\n");
}

UserManager::~UserManager() { delete redis_; }

bool UserManager::loadUsers() {
    std::ifstream file(userFile);
    if (!file) {
        // 文件不存在，创建空文件
        std::ofstream newFile(userFile);
        if (!newFile) return false;
        newFile << "{}";
        newFile.close();
        return true;
    }

    json users;
    try {
        file >> users;
        file.close();
        for (auto& [username, password] : users.items()) 
            redis_->set("user:" + username, password.get<std::string>());
    } catch (...) {
        return false;
    }

    return true;
}

bool UserManager::addUser(const std::string& username, const std::string& password) {
    if (!redis_->get("user:" + username).empty()) return false;
    if (!redis_->set("user:" + username, password)) return false;
    
    // 读取现有文件内容
    std::ifstream file(userFile);
    json users;
    if (file) {
        try {
            file >> users;
        } catch (...) {
            users = json::object();
        }
        file.close();
    } 
    else users = json::object();

    // 添加新用户
    users[username] = password;

    // 写回文件
    std::ofstream outFile(userFile);
    if (!outFile) return false;

    outFile << users.dump(4);
    outFile.close();
    return true;
}

std::string UserManager::getPassword(const std::string& username) { return redis_->get("user:" + username); }
bool UserManager::userExists(const std::string& username) { return !redis_->get("user:" + username).empty(); }
bool UserManager::verifyUser(const std::string& username, const std::string& password) {
    std::string storedPassword = redis_->get("user:" + username);
    return !storedPassword.empty() && storedPassword == password;
}
