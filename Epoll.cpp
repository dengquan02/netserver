#include "Epoll.h"

Epoll::Epoll() // 创建 epoll_fd_
{
    if ((epoll_fd_ = epoll_create(1)) == -1) // 创建epoll句柄（红黑树）。
    {
        printf("epoll_create() failed(%d).\n", errno);
        exit(-1);
    }
}

Epoll::~Epoll() // 关闭 epoll_fd_
{
    close(epoll_fd_);
}

// 把Channel添加/更新到红黑树上。Channel中有fd和需要监视的事件
void Epoll::updateChannel(Channel *ch)
{
    epoll_event ev;           // 声明事件的数据结构
    ev.data.ptr = ch;         // 指定事件的自定义数据
    ev.events = ch->events(); // 指定事件

    if (ch->inEpoll())
    {
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, ch->fd(), &ev) == -1)
        {
            printf("epoll_ctl() failed(%d).\n", errno);
            exit(-1);
        }
    }
    else
    {
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, ch->fd(), &ev) == -1)
        {
            printf("epoll_ctl() failed(%d).\n", errno);
            exit(-1);
        }
        ch->setInEpoll();
    }
}

// 从红黑树上删除Channel
void Epoll::removeChannel(Channel *ch)
{
    if (ch->inEpoll())
    {
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, ch->fd(), 0) == -1)
        {
            printf("epoll_ctl() failed(%d).\n", errno);
            exit(-1);
        }
    }
}

// 运行epoll_wait(), 等待事件发生，已发生的事件用vector返回
std::vector<Channel *> Epoll::poll(int timeout)
{
    std::vector<Channel *> channels;
    bzero(events_, sizeof(events_));
    int in_fds = epoll_wait(epoll_fd_, events_, kMaxEvents, timeout); // 等待监视的fd有事件发生。

    // 返回失败。
    if (in_fds < 0)
    {
        /* 属于程序的bug */
        // EBADF ：epfd不是一个有效的描述符。
        // EFAULT ：参数events指向的内存区域不可写。
        // EINVAL ：epfd不是一个epoll文件描述符，或者参数maxevents小于等于0。
        /* 信号 */
        // EINTR ：阻塞过程中被信号中断，epoll_pwait()可以避免，或者错误处理中，解析error后重新调用epoll_wait()。
        // 在Reactor模型中，不建议使用信号，因为信号处理起来很麻烦，没有必要。------ 陈硕
        perror("epoll_wait() failed");
        exit(-1);
    }
    // 超时。
    if (in_fds == 0)
    {
        return channels;
    }

    for (int i = 0; i < in_fds; ++i) // 遍历epoll返回的数组evs。
    {
        Channel *ch = (Channel *)events_[i].data.ptr; // 取出已发生事件的channel
        ch->setRevents(events_[i].events);            // 设置channel的revents_成员
        channels.push_back(ch);
    }
    return channels;
}