// 文件响应处理器实现
#include "../../include/client/FileHandler.h"
#include "../../include/client/Utils.h"
#include <stdio.h>
#include <fstream>
#include <string>
#include <time.h>

// 外部全局变量声明
extern ClientState state;
extern std::string token;

void FileHandler::handle(const HttpResponse& response) {
    if (response.getStatusCode() == 401) { // 处理 401 Unauthorized 错误
        printf("Token无效，请重新登录\n");
        token = "";
        state = AUTH_STATE; // 切换到认证状态
        return;
    }
    
    // 获取Content-Disposition头
    std::string contentDisposition = response.getContentDisposition();
    
    // 根据Content-Disposition字段值进行分支处理
    if (contentDisposition == "inline") handle_html(response);
    else if (contentDisposition == "attachment") handle_file(response);
    else handle_html(response); // 默认处理为 HTML
}

void FileHandler::handle_file(const HttpResponse& response) {
    std::string fileName = response.getFilenameHeader(); // 提取文件名
    createDirectory(STATIC_FILE_DIR);
    std::string savePath;
    if (!fileName.empty()) savePath = STATIC_FILE_DIR + "/" + fileName; 
    else savePath = STATIC_FILE_DIR + "/response_" + std::to_string(time(nullptr)) + ".txt"; // 如果没有文件名，使用默认命名
    std::ofstream outFile(savePath);
    if (outFile.is_open()) {
        outFile << response.getBody();
        outFile.close();
        printf("\n文件已保存至: %s\n", savePath.c_str());
    } 
    else printf("\n保存文件失败: %s\n", savePath.c_str());
}

void FileHandler::handle_html(const HttpResponse& response) {
    printf("\nHTML内容:\n");
    printf("%s\n", response.getBody().c_str());
}