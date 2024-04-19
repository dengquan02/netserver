// 功能：持续循环的获取监听结果并且根据结果调用处理函数

// 作为一个网络服务器，需要有持续监听、持续获取监听结果、持续处理监听结果对应的事件的能力，
// 也就是我们需要循环的去调用Epoll:loop方法获取实际发生事件的Channel集合，
// 然后调用这些Channel里面保管的不同类型事件的处理函数（调用Channel::handleEvent方法）。

// EventLoop就是负责实现“循环”，负责驱动“循环”的重要模块！！
// Channel和Poller其实相当于EventLoop的手下，EventLoop整合封装了二者并向上提供了更方便的接口来使用。

// 一个EventLoop就表示一个server线程，有唯一的epoll句柄（红黑树），可以对应多个Channel，负责事件循环

#pragma once
#include "Epoll.h"
#include <memory>
#include <unistd.h>
#include <sys/syscall.h>
#include <queue>
#include <mutex>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <map>
#include <atomic>
#include "Connection.h"

// 前置声明解决两个类的互相依赖。而使用前置声明要注意两个“陷阱”
class Channel;
class Epoll;
class Connection;
using spConnection = std::shared_ptr<Connection>;

class EventLoop
{
private:
    std::unique_ptr<Epoll> ep_;                                    // 每个事件循环只有一个Epoll
    std::function<void(EventLoop *)> epoll_wait_timeout_callback_; // epoll_wait()超时的回调函数

    pid_t thread_id_;                              // 事件循环所在线程的id
    std::queue<std::function<void()>> task_queue_; // 事件循环被eventfd唤醒后执行的任务队列
    std::mutex mutex_;                             // 任务队列同步的互斥锁
    int wakeup_fd_;                                // 用于唤醒事件循环线程的eventfd
    std::unique_ptr<Channel> wake_channel_;        // eventfd的Channel

    int timetvl_;                            // 闹钟时间间隔
    int timeout_;                            // Connection对象的超时时间
    int timer_fd_;                           // 定时器fd
    std::unique_ptr<Channel> timer_channel_; // 定时器的Channel
    bool is_mainloop_;
    std::mutex conns_mutex_;                        // 保护conns_的互斥锁
    std::map<int, spConnection> conns_;             // 存放运行在该事件循环上的全部Connection对象
    std::function<void(int)> remove_conn_callback_; // 清理空闲连接的回调函数

    std::atomic_bool stop_; // 初始化为false，如果为true表示停止事件循环

public:
    EventLoop(bool is_mainloop, int timetvl = 30, int timeout = 80); // 创建Epoll对象
    ~EventLoop();

    void run();  // 运行事件循环
    void stop(); // 停止事件循环

    void updateChannel(Channel *ch); // 把channel添加/更新到红黑树上
    void setEpollWaitTimeoutCallback(std::function<void(EventLoop *)> func);
    void removeChannel(Channel *ch); // 从红黑树上删除Channel

    bool isInLoopThread(); // 判断当前线程是否为事件循环线程

    void queueInLoop(std::function<void()> func); // 将任务添加到队列中
    void wakeup();                                // 用eventfd唤醒事件循环线程
    void handleWakeup();                          // 被唤醒后执行的函数

    void handleTimer(); // 闹钟响时执行的函数

    void newConnection(spConnection conn); // 把Connection对象保存在conns_中
    void setRemoveConnCallback(std::function<void(int)> func);
};
