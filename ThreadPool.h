#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <queue>
#include <sys/syscall.h>
#include <mutex>
#include <unistd.h>
#include <thread>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

class ThreadPool
{
private:
    std::vector<std::thread> threads_;             // 线程池中的线程
    std::queue<std::function<void()>> task_queue_; // 任务队列
    std::mutex mutex_;                             // 任务队列同步的互斥锁
    std::condition_variable condition_;            // 任务队列同步的条件变量
    std::atomic_bool stop_;                        // 在析构函数中，将stop_设置为true，所有线程将退出
    std::string thread_type_;                      // 线程种类："IO" "WORKS"

public:
    // 在构造函数中启动thread_num个线程
    ThreadPool(size_t thread_num, const std::string &thread_type);

    // 将任务添加到队列中
    void addTask(std::function<void()> task);

    size_t size();

    // 停止线程
    void stop();

    // 在析构函数中停止线程
    ~ThreadPool();
};
