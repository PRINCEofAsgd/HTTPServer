// 客户端工具函数实现
#include "../../include/client/Utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <fstream>

// 静态文件保存目录
const std::string STATIC_FILE_DIR = "/home/loki/桌面/HttpStaticFiles/publicFiles";

// 服务器地址和端口
const std::string SERVER_IP = "192.168.1.128";
const std::string SERVER_PORT = "8079";

// 心跳检测相关常量
const int HEARTBEAT_INTERVAL = 20; // 心跳检测时间间隔（秒）

// 全局变量
ClientState state = AUTH_STATE;
std::string token = "";
int requestId_ = 1000;

void stop(int sig) {
    printf("收到信号 %d, 退出程序\n", sig);
    exit(0);
}

// 创建目录
void createDirectory(const std::string& dirPath) { // 递归创建目录
    struct stat statBuf;
    if (stat(dirPath.c_str(), &statBuf) != 0) { // 目录不存在，创建它
        if (mkdir(dirPath.c_str(), 0755) != 0) { // 如果创建失败，可能是因为父目录不存在，尝试递归创建
            size_t pos = dirPath.find_last_of('/');
            if (pos != std::string::npos) {
                std::string parentDir = dirPath.substr(0, pos);
                createDirectory(parentDir);
                // 再次尝试创建目录
                if (mkdir(dirPath.c_str(), 0755) != 0) 
                    printf("创建目录失败: %s, 错误: %s\n", dirPath.c_str(), strerror(errno));
            }
        }
    } 
    else if (!S_ISDIR(statBuf.st_mode)) printf("路径已存在但不是目录: %s\n", dirPath.c_str()); // 路径存在但不是目录
}

// 用户输入处理函数
void getFileInput(std::string& input, const std::string& prompt) {
    std::cout << "\n" << prompt << ": ";
    std::getline(std::cin, input);
}

void getUserInput(std::string& username, std::string& password) {
    std::cout << "\n请输入用户名: ";
    std::getline(std::cin, username);
    
    while (username.empty()) { // 输入验证
        std::cout << "用户名不能为空，请重新输入: ";
        std::getline(std::cin, username);
    }
    
    std::cout << "请输入密码: ";
    std::getline(std::cin, password);
    
    while (password.empty()) { // 输入验证
        std::cout << "密码不能为空，请重新输入: ";
        std::getline(std::cin, password);
    }
}
