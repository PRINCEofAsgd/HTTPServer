// HTTP客户端程序
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

// 导入模块
#include "../../include/client/HandlerFactory.h"
#include "../../include/client/Utils.h"
#include "../../include/http/HttpRequest.h"
#include "../../include/client/HandlerRouter.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 1) {
        printf("usage:./main_client\n"); 
        printf("example:./main_client\n"); 
        return -1;
    }

    signal(SIGTERM, stop);
    signal(SIGINT, stop);

    HandlerFactory handler_factory;

    while (true) {
        HttpRequest request;
        if (state == AUTH_STATE) { // 用户认证分支，提供认证功能选择
            printf("\n请选择操作:\n");
            printf("0. 刷新\n");
            printf("1. 登录\n");
            printf("2. 注册\n");
            printf("3. 退出\n");
            printf("请输入选项: \n");
            int choice;
            cin >> choice;
            cin.ignore(); // 忽略换行符
            string username, password;
            
            if (choice == 1) { // 登录分支
                getUserInput(username, password); // 获取登录信息
                // 自增请求ID
                requestId_++;
                // 构造登录请求
                request.setMethod("POST");
                request.setPath("/login");
                request.setVersion("HTTP/1.1");
                request.addHeader("Host", SERVER_IP + ":" + SERVER_PORT);
                request.addHeader("Connection", "keep-alive");
                request.addHeader("Content-Type", "application/json");
                request.addHeader("X-Request-ID", to_string(requestId_));
                string jsonBody = "{\"username\":\"" + username + "\",\"password\":\"" + password + "\"}";
                request.addHeader("Content-Length", to_string(jsonBody.size()));
                request.setBody(jsonBody);
                // 注册路由
                handler_factory.RegisterRouter(requestId_, ReqType::LOGIN);
                // 使用ClientConn发送请求
                handler_factory.handleRequest(request);
            }

            else if (choice == 2) { // 注册分支
                getUserInput(username, password); // 获取注册信息
                // 自增请求ID
                requestId_++;
                // 构造注册请求
                request.setMethod("POST");
                request.setPath("/register");
                request.setVersion("HTTP/1.1");
                request.addHeader("Host", SERVER_IP + ":" + SERVER_PORT);
                request.addHeader("Connection", "keep-alive");
                request.addHeader("Content-Type", "application/json");
                request.addHeader("X-Request-ID", to_string(requestId_));
                string jsonBody = "{\"username\":\"" + username + "\",\"password\":\"" + password + "\"}";
                request.addHeader("Content-Length", to_string(jsonBody.size()));
                request.setBody(jsonBody);
                // 注册路由
                handler_factory.RegisterRouter(requestId_, ReqType::REGISTER);
                // 使用ClientConn发送请求
                handler_factory.handleRequest(request);
            } 

            else if (choice == 3) { // 退出分支
                printf("退出程序\n");
                break;
            } 
            else if (choice == 0) {
                printf("刷新页面\n");
                continue;
            }
            else printf("无效选项\n"); // 无效选项
        } 
        
        else if (state == BUSINESS_STATE) { // 用户业务分支，提供业务功能选择
            printf("\n请选择操作:\n");
            printf("0. 刷新\n");
            printf("1. 获取静态资源\n");
            printf("2. 下载个人文件\n");
            printf("3. 上传个人文件\n");
            printf("4. 登出\n");
            printf("请输入选项: \n");
            int choice;
            cin >> choice;
            cin.ignore(); // 忽略换行符
            string fileName;
            
            if (choice == 1) { // 获取静态资源分支
                getFileInput(fileName, "请输入静态文件名");
                // 自增请求ID
                requestId_++;
                // 构造获取静态资源请求
                request.setMethod("GET");
                request.setPath("/");
                if (!fileName.empty()) request.addQueryParameter("filename", fileName);
                request.setVersion("HTTP/1.1");
                request.addHeader("Host", SERVER_IP + ":" + SERVER_PORT);
                request.addHeader("Connection", "keep-alive");
                request.addHeader("X-Request-ID", to_string(requestId_));
                // 注册路由
                handler_factory.RegisterRouter(requestId_, ReqType::GET_STATIC);
                // 使用 ClientConn 发送请求
                handler_factory.handleRequest(request);
            }

            else if (choice == 2) { // 下载个人文件分支
                getFileInput(fileName, "请输入要下载的文件名");
                // 自增请求ID
                requestId_++;
                // 构造下载个人文件请求
                request.setMethod("GET");
                request.setPath("/download");
                if (!fileName.empty()) request.addQueryParameter("filename", fileName);
                request.setVersion("HTTP/1.1");
                request.addHeader("Host", SERVER_IP + ":" + SERVER_PORT);
                request.addHeader("Connection", "keep-alive");
                request.addHeader("X-Request-ID", to_string(requestId_));
                if (!token.empty()) request.addHeader("Authorization", "Bearer " + token);
                // 注册路由
                handler_factory.RegisterRouter(requestId_, ReqType::DOWNLOAD_FILE);
                // 使用 ClientConn 发送请求
                handler_factory.handleRequest(request);
            }

            else if (choice == 3) { // 上传个人文件分支
                getFileInput(fileName, "请输入待上传的文件路径");
                // 打开文件
                int file_fd = open(fileName.c_str(), O_RDONLY);
                if (file_fd < 0) {
                    printf("错误: 无法打开文件 %s\n", fileName.c_str());
                    continue;
                }
                // 获取文件大小
                struct stat file_stat;
                if (stat(fileName.c_str(), &file_stat) < 0) {
                    printf("错误: 无法获取文件信息 %s\n", fileName.c_str());
                    close(file_fd);
                    continue;
                }
                size_t file_size = file_stat.st_size;
                // 提取文件名
                size_t slashPos = fileName.find_last_of('/');
                string uploadFileName = fileName;
                if (slashPos != string::npos) uploadFileName = fileName.substr(slashPos + 1);
                // 自增请求ID
                requestId_++;
                // 构造上传文件请求
                request.setMethod("POST");
                request.setPath("/upload");
                request.setVersion("HTTP/1.1");
                request.addHeader("Host", SERVER_IP + ":" + SERVER_PORT);
                request.addHeader("Connection", "keep-alive");
                request.addHeader("Content-Type", "application/octet-stream");
                request.addHeader("Content-Length", to_string(file_size));
                request.addHeader("X-Filename", uploadFileName);
                request.addHeader("X-Request-ID", to_string(requestId_));
                if (!token.empty()) request.addHeader("Authorization", "Bearer " + token);
                // 显式设置文件相关参数
                request.setUseSendfile(true);
                request.setFileFd(file_fd);
                request.setFileOffset(0);
                request.setFileSize(file_size);
                // 注册路由
                handler_factory.RegisterRouter(requestId_, ReqType::UPLOAD_FILE);
                // 使用 ClientConn 发送请求
                handler_factory.handleRequest(request);
            } 

            else if (choice == 4) { // 登出分支
                printf("登出成功\n");
                token = "";
                state = AUTH_STATE; // 切换到认证状态
            } 
            else if (choice == 0) {
                printf("刷新页面\n");
                continue;
            }
            else printf("无效选项\n"); // 无效选项
        }
    }

    return 0;
}