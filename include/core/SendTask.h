#pragma once
#include <string>
#include <memory>

// 任务状态枚举
enum class TaskStatus {
    PENDING,   // 任务待处理
    PROCESSING, // 任务处理中
    COMPLETED, // 任务处理完成
    FAILED     // 任务处理失败
};

// 发送任务类型枚举
enum class TaskType {
    BUFFER, // 缓冲区类型任务
    FILE    // 文件类型任务
};

// 发送任务结构体
struct SendTask {
    TaskType type;     // 任务类型
    TaskStatus status; // 任务状态
    // 缓冲区类型任务数据
    std::shared_ptr<std::string> buffer_data; // 缓冲区数据
    // 文件类型任务数据
    int file_fd;         // 文件描述符
    off_t file_offset;   // 文件偏移量
    size_t file_size;    // 文件大小
    
    SendTask(TaskType t) : type(t), status(TaskStatus::PENDING), 
                          buffer_data(nullptr), file_fd(-1), 
                          file_offset(0), file_size(0) {}
};
