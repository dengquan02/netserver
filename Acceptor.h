/* 在Channel之上再做一层封装：监听Channel封装成Acceptor类 */

#pragma once
#include <functional>
#include "EventLoop.h"
#include "InetAddress.h"
#include "Socket.h"
#include "Channel.h"
#include <memory>

class Acceptor
{
private:
    EventLoop *loop_; // 对应的事件循环，在构造函数中传入
    // accept_sock_和accept_channel_（一个网络服务程序只有一个）占用空间很小，可以不用指针，使用栈内存
    Socket accept_sock_;                                                   // 服务端用于监听的socket，在构造函数中创建
    Channel accept_channel_;                                               // 对应的Channel，在构造函数中创建
    std::function<void(std::unique_ptr<Socket>)> new_connection_callback_; // 处理新客户端连接请求的回调函数，将回调TcpServer::NewConnection

public:
    Acceptor(EventLoop *loop, const std::string &ip, const uint16_t port);
    ~Acceptor();

    void newConnection(); // 处理新的客户端连接

    void setNewConnectionCallback(std::function<void(std::unique_ptr<Socket>)> func);
};
