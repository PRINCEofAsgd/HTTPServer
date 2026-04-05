// 客户端工具函数
#pragma once
#include <string>

// 静态文件保存目录
extern const std::string STATIC_FILE_DIR;

// 服务器地址和端口
extern const std::string SERVER_IP;
extern const std::string SERVER_PORT;

// 心跳检测相关常量
extern const int HEARTBEAT_INTERVAL; // 心跳检测时间间隔（秒）

// 状态枚举
enum ClientState {
    AUTH_STATE,    // 认证状态
    BUSINESS_STATE // 业务状态
};

// 全局变量
extern ClientState state;
extern std::string token;
extern int requestId_;

// 信号处理函数
void stop(int sig);

// 创建目录
void createDirectory(const std::string& dirPath);

// 用户输入处理函数
void getFileInput(std::string& input, const std::string& prompt);
void getUserInput(std::string& username, std::string& password);

// 业务流程封装函数
class HandlerFactory;

void ProcessGetStatic(HandlerFactory& factory);
void ProcessLogin(HandlerFactory& factory);
void ProcessRegister(HandlerFactory& factory);
void ProcessDownloadFile(HandlerFactory& factory);
void ProcessUploadFile(HandlerFactory& factory);
