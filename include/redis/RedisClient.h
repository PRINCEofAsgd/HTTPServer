#ifndef REDIS_CLIENT_H
#define REDIS_CLIENT_H

#include <hiredis/hiredis.h>
#include <string>

class RedisClient {
private:
    redisContext* context_; // Redis 服务器连接上下文指针

public:
    RedisClient();
    ~RedisClient();

    bool connect(const std::string& host = "127.0.0.1", int port = 6379, int timeout_ms = 1000); // 连接 Redis 服务器
    bool isConnected() const;         // 检查是否已连接
    redisContext* getContext() const; // 获取 Redis 连接上下文指针

    // Redis 基础操作
    std::string get(const std::string& key);                                    // 获取键值
    bool set(const std::string& key, const std::string& value);                 // 设置键值
    bool setex(const std::string& key, int seconds, const std::string& value);  // 设置带过期时间的键值
    bool del(const std::string& key);                                           // 删除键
};

#endif // REDIS_CLIENT_H
