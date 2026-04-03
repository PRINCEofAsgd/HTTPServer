#include "../../include/redis/RedisClient.h"
#include <cstring>

RedisClient::RedisClient() : context_(nullptr) {}
RedisClient::~RedisClient() { if (context_) redisFree(context_); }

bool RedisClient::connect(const std::string& host, int port, int timeout_ms) {
    struct timeval timeout = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    context_ = redisConnectWithTimeout(host.c_str(), port, timeout);
    if (context_ == nullptr || context_->err) {
        if (context_) {
            redisFree(context_);
            context_ = nullptr;
        }
        return false;
    }
    return true;
}

bool RedisClient::isConnected() const { return context_ != nullptr && context_->err == 0; }
redisContext* RedisClient::getContext() const { return context_; }

std::string RedisClient::get(const std::string& key) {
    if (!isConnected()) return "";
    
    // GET key
    redisReply* reply = (redisReply*)redisCommand(context_, "GET %s", key.c_str());
    if (reply == nullptr || reply->type != REDIS_REPLY_STRING) {
        if (reply) freeReplyObject(reply);
        return "";
    }
    
    std::string value(reply->str, reply->len);
    freeReplyObject(reply);
    return value;
}

bool RedisClient::set(const std::string& key, const std::string& value) {
    if (!isConnected()) return false;
    
    // SET key value
    redisReply* reply = (redisReply*)redisCommand(context_, "SET %s %s", key.c_str(), value.c_str());
    if (reply == nullptr || reply->type != REDIS_REPLY_STATUS || strcmp(reply->str, "OK") != 0) {
        if (reply) freeReplyObject(reply);
        return false;
    }
    
    freeReplyObject(reply);
    return true;
}

bool RedisClient::setex(const std::string& key, int seconds, const std::string& value) {
    if (!isConnected()) return false;
    
    // SETEX key ttl value
    redisReply* reply = (redisReply*)redisCommand(context_, "SETEX %s %d %s", key.c_str(), seconds, value.c_str());
    if (reply == nullptr || reply->type != REDIS_REPLY_STATUS || strcmp(reply->str, "OK") != 0) {
        if (reply) freeReplyObject(reply);
        return false;
    }
    
    freeReplyObject(reply);
    return true;
}

bool RedisClient::del(const std::string& key) {
    if (!isConnected()) return false;
    
    // DEL key
    redisReply* reply = (redisReply*)redisCommand(context_, "DEL %s", key.c_str());
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
        if (reply) freeReplyObject(reply);
        return false;
    }
    
    bool success = reply->integer > 0;
    freeReplyObject(reply);
    return success;
}
