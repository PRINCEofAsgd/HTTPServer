#include "../../include/core/ThreadPool.h"

ThreadPool::ThreadPool(size_t threadnum, const std::string& threadtype) : threadtype_(threadtype), stop_(false) {
    for (size_t ii = 0; ii < threadnum; ii++) { // 启动 threadnum 个线程
        threads_.emplace_back([this] {          // 构造执行 lambda 函数的线程，并把线程对象存入 threads_ 容器中
            while (!stop_) {                    // 线程池停止之前，线程一直在循环中等待任务
                std::function<void()> task;     // 用于存放出队的元素
                {                               // 锁作用域的开始
                    std::unique_lock<std::mutex> lock(this->mutex_);                // unique_lock 能自动解锁地管理 mutex，并接受 condition_ 的 wait() 方法调用
                    this->condition_.wait(lock, [this] {                            // 解锁并阻塞，等待条件变量唤醒线程
                        return !this->taskqueue_.empty() || this->stop_ == true;    // 设置线程被条件变量唤醒需要的其他条件
                    });
                    if (this->stop_ == true && this->taskqueue_.empty()) break;     // 线程池停止即退出线程等待循环，若队列中还有任务则执行完再退出
                    task = std::move(this->taskqueue_.front());                     // 取出一个任务
                    this->taskqueue_.pop();                                         // 出队一个任务
                }       // 锁作用域的结束，自动释放锁，也可手动释放
                task(); // 执行任务
            }
        });
    }
}

ThreadPool::~ThreadPool() { stop(); }

void ThreadPool::addtask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_); // 也可以用 unique_lock, lock_guard 只是不能手动释放锁但更轻量
        taskqueue_.push(task); 
    }
    condition_.notify_one(); // 一个任务只需唤醒一个线程，避免惊群浪费系统调度资源，分配给哪个线程由操作系统内核调度，会保证任务公平调度
}

void ThreadPool::stop() {
    if (stop_) return;
    stop_ = true;                           // 设置线程池停止标志，通知线程退出循环
    condition_.notify_all();                // 唤醒全部线程，让它们检查 stop_ 标志并退出循环
    for (auto& th : threads_) th.join();    // 等待全部线程执行完任务后退出
}