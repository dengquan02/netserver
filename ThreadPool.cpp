#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t thread_num, const std::string &thread_type) : stop_(false), thread_type_(thread_type)
{
    // 启动thread_num个线程，将阻塞在条件变量上
    for (size_t i = 0; i < thread_num; ++i)
    {
        // 用lambda表达式创建线程   
        // emplace_back支持原地构造
        threads_.emplace_back([this]() 
        {
            printf("create %s thread(%ld).\n", thread_type_.c_str(), syscall(SYS_gettid));

            // while (!stop_)
            while (!stop_ || !task_queue_.empty())
            {
                std::function<void()> task; // 存放出队的元素

                { /* 获取锁 */
                    std::unique_lock<std::mutex> lock(mutex_);

                    // 等待生产者的条件变量。        
                    condition_.wait(lock, [this]() 
                    {
                        return stop_ || !task_queue_.empty();
                    });

                    // 线程池要停止，如果队列没有任务，可以立马退出
                    if (stop_ && task_queue_.empty())
                        return;

                    task = std::move(task_queue_.front()); // 采用移动语义，避免拷贝
                    task_queue_.pop();
                } /* 释放锁 */
                
                // printf("%s(%ld) execute task.\n", thread_type_.c_str(), syscall(SYS_gettid));
                task(); // 执行任务（真正的启动了）
            }
        });
    }
}

// 将任务添加到队列中
void ThreadPool::addTask(std::function<void()> task)
{
    if (stop_) return; // 如果已经调用析构函数了，那么就不能继续添加任务了
    { /* 获取锁 */
        std::lock_guard<std::mutex> lock(mutex_);
        task_queue_.push(task);
    } /* 释放锁 */

    condition_.notify_one();
}

size_t ThreadPool::size()
{
    return threads_.size();
}

// 停止线程
void ThreadPool::stop()
{
    if (stop_) return;
    stop_ = true;

    condition_.notify_all(); // 唤醒全部线程

    // 等待全部线程执行完任务后退出
    for (std::thread &th : threads_)
    {
        if(th.joinable()) {
            th.join();
        }
    }
}

// 在析构函数中停止线程
ThreadPool::~ThreadPool()
{
    stop();
}
