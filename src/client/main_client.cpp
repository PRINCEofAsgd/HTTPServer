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
#include <limits>

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
        if (state == AUTH_STATE) { // 用户认证分支，提供认证功能选择
            printf("\n请选择操作:\n");
            printf("0. 刷新\n");
            printf("1. 获取静态资源\n");
            printf("2. 登录\n");
            printf("3. 注册\n");
            printf("4. 退出\n");
            printf("请输入选项: \n");
            int choice;
            cin >> choice;
            
            // 检查输入是否有效
            if (cin.fail()) {
                cin.clear(); // 清除错误状态
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // 忽略当前行的所有输入
                printf("输入无效，请重新输入数字选项\n");
                continue;
            }
            
            cin.ignore(); // 忽略换行符
            
            if (choice == 1) ProcessGetStatic(handler_factory);     // 获取静态资源分支
            else if (choice == 2) ProcessLogin(handler_factory);    // 登录分支
            else if (choice == 3) ProcessRegister(handler_factory); // 注册分支
            else if (choice == 4) break;                       // 退出
            else if (choice == 0) printf("刷新页面\n"); // 刷新页面
            else printf("无效选项\n");                  // 无效选项
        } 
        
        else if (state == BUSINESS_STATE) { // 用户业务分支，提供业务功能选择
            printf("\n请选择操作:\n");
            printf("0. 刷新\n");
            printf("1. 下载个人文件\n");
            printf("2. 上传个人文件\n");
            printf("3. 登出\n");
            printf("请输入选项: \n");
            int choice;
            cin >> choice;
            
            // 检查输入是否有效
            if (cin.fail()) {
                cin.clear(); // 清除错误状态
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // 忽略当前行的所有输入
                printf("输入无效，请重新输入数字选项\n");
                continue;
            }
            
            cin.ignore(); // 忽略换行符

            if (choice == 1) ProcessDownloadFile(handler_factory);      // 下载个人文件分支
            else if (choice == 2) ProcessUploadFile(handler_factory);   // 上传个人文件分支
            else if (choice == 3) { // 登出
                printf("登出成功\n");
                token = "";
                state = AUTH_STATE; // 切换到认证状态
            } 
            else if (choice == 0) printf("刷新页面\n"); // 刷新页面
            else printf("无效选项\n");                  // 无效选项
        }
    }

    return 0;
}