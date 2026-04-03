#pragma once
#include <string>
#include <vector>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPool {
private:
    std::vector<std::thread> threads_;              // 存放、管理线程的容器
    std::queue<std::function<void()>> taskqueue_;   // 任务队列，存放待执行的任务
    std::mutex mutex_;                              // 保护任务队列的互斥锁
    std::condition_variable condition_;             // 条件变量，用于任务到来时唤醒线程
    std::string threadtype_;                        // 线程类型，"IO" 或 "WORKS"，用于调试输出
    std::atomic<bool> stop_;                        // 线程池停止标志位，析构函数中设置为 true，通知线程退出循环

public:
    ThreadPool(size_t threadnum, const std::string& threadtype);    // 启动 threadnum 个线程
    ~ThreadPool();                                                  // 析构函数中停止线程池，置标志位为 true, 线程读取到后停止

    void addtask(std::function<void()> task);                       // 添加任务到队列中，唤醒线程处理
    void stop();
};
