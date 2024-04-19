// Channel 相当于一个文件描述符的保姆，包含的信息有：
//   socketfd(文件描述符)，对应的epoll句柄(事件循环)，监视的事件（events）

// 当 revents 被设置时，表时在执行 epoll_wait() 时发生的事件，handleEvent根据它可以进行相应处理。
// 注意，handleEvent中调用了回调函数，可根据是监听Socket还是已连接Socket使用不同的处理函数

#pragma once
#include <sys/epoll.h>
#include <functional>
#include "InetAddress.h"
#include "Socket.h"
#include "EventLoop.h"
#include <memory>

class EventLoop; // 前置声明

class Channel
{
private:
    int fd_ = -1;                          // Channel对应的socketfd
    EventLoop *loop_;                      // Channel对应的事件循环，Channel与EventLoop是多对一的关系，一个Channel只对应一个EventLoop。
    bool in_epoll_ = false;                // Channle是否已添加到epoll树上。未添加的话，调用epoll_ctl()时用EPOLL_CTL_ADD，否则用EPOLL_CTL_MOD
    uint32_t events_ = 0;                  // fd_需要监视的事件。listenfd和clientfd都需要监视EPOLLIN，clientfd可能还要监视EPOLLOUT
    uint32_t revents_ = 0;                 // fd_已发生的事件
    std::function<void()> read_callback_;  // fd_读事件的回调函数，如果是accept_channel，将回调Acceptor::NewConnection；如果是connect_channel，将回调Channel::OnMessage()。
    std::function<void()> close_callback_; // 关闭fd_的回调函数，将回调Connection::CloseConnection
    std::function<void()> error_callback_; // fd_发生错误的回调函数，将回调Connection::ErrorConnection
    std::function<void()> write_callback_; // fd_写事件的回调函数，将回调Connection::Write

public:
    Channel(EventLoop *loop, int fd);
    ~Channel();

    int fd();
    void useEt();                 // 采用边缘触发

    // 将Channel中的文件描述符及其感兴趣事件在事件监听器上注册或移除
    void enableReading();         // 监视fd_的读事件
    void disableReading();        // 取消监视fd_的读事件
    void enableWriting();         // 监视fd_的写事件
    void disableWriting();        // 取消监视fd_的写事件
    void disableAll();            // 取消监视全部事件

    void remove();                // 从事件循环中删除Channel
    void setInEpoll();            // 把in_epoll_设置为true
    void setRevents(uint32_t ev); // 设置revents的值为ev
    bool inEpoll();
    uint32_t events();
    uint32_t revents();

    void handleEvent(); // 事件处理函数：epoll_wait()返回时执行它

    // 向Channel对象注册各类事件的处理函数
    void setReadCallback(std::function<void()>); // 设置fd_读事件的回调函数
    void setCloseCallback(std::function<void()> func);
    void setErrorCallback(std::function<void()> func);
    void setWriteCallback(std::function<void()> func);
};