#include "EventLoop.h"

// 创建定时器
int createTimerfd(int sec = 30)
{
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK); // 创建timerfd。
    struct itimerspec timeout;                                             // 定时时间的数据结构。
    memset(&timeout, 0, sizeof(struct itimerspec));
    timeout.it_value.tv_sec = sec; // 定时时间
    timeout.it_value.tv_nsec = 0;
    timerfd_settime(tfd, 0, &timeout, 0); // 开始计时
    return tfd;
}

// 创建Epoll对象
EventLoop::EventLoop(bool is_mainloop, int timetvl, int timeout)
    : ep_(new Epoll()), wakeup_fd_(eventfd(0, EFD_NONBLOCK)), wake_channel_(new Channel(this, wakeup_fd_)),
      timer_fd_(createTimerfd(timetvl_)), timer_channel_(new Channel(this, timer_fd_)), is_mainloop_(is_mainloop),
      timetvl_(timetvl), timeout_(timeout), stop_(false)
{
    wake_channel_->setReadCallback(std::bind(&EventLoop::handleWakeup, this));
    wake_channel_->enableReading();

    timer_channel_->setReadCallback(std::bind(&EventLoop::handleTimer, this));
    timer_channel_->enableReading();
}

EventLoop::~EventLoop()
{
}

// 运行事件循环
void EventLoop::run()
{
    thread_id_ = syscall(SYS_gettid); // 获取事件循环所在线程的id
    while (!stop_)
    {
        std::vector<Channel *> channels = ep_->poll(10 * 1000);

        if (channels.empty())
            epoll_wait_timeout_callback_(this);
        else
            for (auto &ch : channels)
            {
                ch->handleEvent();
            }
    }
}

// 停止事件循环
void EventLoop::stop()
{
    stop_ = true;
    wakeup(); // 唤醒事件循环。如果没有这行代码，事件循环只有在下次闹钟响或epoll_wait()超时时才会停下来
}

// 把channel添加/更新到红黑树上
void EventLoop::updateChannel(Channel *ch)
{
    ep_->updateChannel(ch);
}

// 从红黑树上删除Channel
void EventLoop::removeChannel(Channel *ch)
{
    ep_->removeChannel(ch);
}

void EventLoop::setEpollWaitTimeoutCallback(std::function<void(EventLoop *)> func)
{
    epoll_wait_timeout_callback_ = func;
}

// 判断当前线程是否为事件循环线程
bool EventLoop::isInLoopThread()
{
    return thread_id_ == syscall(SYS_gettid);
}

// 将任务添加到队列中
void EventLoop::queueInLoop(std::function<void()> func)
{
    std::lock_guard<std::mutex> gd(mutex_);
    task_queue_.push(func);

    wakeup(); // 唤醒事件循环
}

// 用eventfd唤醒事件循环线程
void EventLoop::wakeup()
{
    uint64_t val = 1;
    write(wakeup_fd_, &val, sizeof(val));
}

// 被唤醒后执行的函数
void EventLoop::handleWakeup()
{
    uint64_t val;
    read(wakeup_fd_, &val, sizeof(val)); // 从eventfd中读取出数据，如果不读取，eventfd的读事件会一直触发（水平触发模式下）

    std::function<void()> func;
    std::lock_guard<std::mutex> gd(mutex_);
    while (!task_queue_.empty())
    {
        func = std::move(task_queue_.front());
        task_queue_.pop();
        func(); // 执行任务
    }
}

// 闹钟响时执行的函数
void EventLoop::handleTimer()
{
    // 重新计时
    struct itimerspec timeout; // 定时时间的数据结构。
    memset(&timeout, 0, sizeof(struct itimerspec));
    timeout.it_value.tv_sec = timetvl_; // 定时时间
    timeout.it_value.tv_nsec = 0;
    timerfd_settime(timer_fd_, 0, &timeout, 0); // 开始计时

    if (is_mainloop_)
    {
    }
    else
    {
        time_t now = time(0); // 获取当前时间

        for (auto it = conns_.begin(); it != conns_.end(); /* no increment */)
        {
            if (it->second->timeout(now, timeout_))
            {
                // erase方法会返回下一个有效元素的迭代器，避免了迭代器失效的问题
                remove_conn_callback_(it->first); // 删除在TcpServer的map中的conn
                {
                    std::lock_guard<std::mutex> gd(conns_mutex_);
                    it = conns_.erase(it); // 删除在事件循环的map中的conn
                }
            }
            else
                ++it;
        }
    }
}

// 把Connection对象保存在conns_中
void EventLoop::newConnection(spConnection conn)
{
    std::lock_guard<std::mutex> gd(conns_mutex_);
    conns_[conn->fd()] = conn;
}

void EventLoop::setRemoveConnCallback(std::function<void(int)> func)
{
    remove_conn_callback_ = func;
}
