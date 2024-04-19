// 负责监听注册的事件是否触发 以及 返回发生事件的Channel（封装了文件描述符fd和具体事件revents）

// epoll的各类操作封装
// The struct epoll_event is defined as:
// typedef union epoll_data {
//     void    *ptr;
//     int      fd;
//     uint32_t u32;
//     uint64_t u64;
// } epoll_data_t;

// struct epoll_event {
//     uint32_t     events;    /* Epoll events */
//     epoll_data_t data;      /* User data variable */
// };

// 事件的自定义数据使用 epoll_event.data.ptr 指向Channel，
// 所以在epoll_wait()返回后设置的events中，每个event的data.ptr就指向了发生该事件的Channel。

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>
#include <string.h>
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>
#include "Channel.h"

class Channel; // 前置声明

class Epoll
{
private:
    static const int kMaxEvents = 100; // epoll_wait()返回事件数组的大小
    int epoll_fd_ = -1;                // epoll句柄
    epoll_event events_[kMaxEvents];   // epoll_wait()返回事件数组

public:
    Epoll();  // 创建 epoll_fd_
    ~Epoll(); // 关闭 epoll_fd_

    void updateChannel(Channel *ch);               // 把Channel添加/更新到红黑树上。Channel中有fd和需要监视的事件
    void removeChannel(Channel *ch);               // 从红黑树上删除Channel
    std::vector<Channel *> poll(int timeout = -1); // 运行epoll_wait(), 等待事件发生，已发生的事件用vector返回
};