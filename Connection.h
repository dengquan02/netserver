/* 在Channel之上再做一层封装：客户端连接的Channel封装成Connection类 */

#pragma once
#include <functional>
#include "EventLoop.h"
#include "InetAddress.h"
#include "Socket.h"
#include "Channel.h"
#include "Buffer.h"
#include "Timestamp.h"
#include <memory>
#include <atomic>
#include <syscall.h>

class EventLoop;
class Channel;
class Connection;
using spConnection = std::shared_ptr<Connection>;

class Connection : public std::enable_shared_from_this<Connection>
{
private:
    EventLoop *loop_;                          // 对应的事件循环，在构造函数中传入
    std::unique_ptr<Socket> connect_sock_;     // 与客户端进行通讯的socket，在构造函数中传入，但生命周期由Connection管理
    std::unique_ptr<Channel> connect_channel_; // 对应的Channel，在构造函数中创建。一个网络服务程序有很多个connnect_channel，使用堆内存
    std::unique_ptr<Buffer> input_buffer_;     // 接收缓冲区
    std::unique_ptr<Buffer> output_buffer_;    // 发送缓冲区
    std::atomic_bool disconnect_;              // 判断连接是否已断开。在IO线程中改变变量的值，在工作线程中判断变量的值，因此使用原子类型

    std::function<void(spConnection)> close_connection_callback_;          // 处理连接关闭的回调函数，将回调TcpServer::CloseConnection
    std::function<void(spConnection)> error_connection_callback_;          // 处理连接错误的回调函数，将回调TcpServer::ErrorConnection
    std::function<void(spConnection, std::string &)> on_message_callback_; // 处理报文的回调函数，将回调TcpServer::OnMessage
    std::function<void(spConnection)> send_complete_callback_;             // 发送数据完毕后，将回调TcpServer::SendComplete
    Timestamp lastatime_;                                                  // 创建Connection对象时设为当前时间，每接收一次报文就更新

public:
    Connection(EventLoop *loop, std::unique_ptr<Socket> client_sock, int sep);
    ~Connection();

    int fd() const;         // 返回已连接（客户端）socket的fd
    std::string ip() const; // 返回客户端的ip
    uint16_t port() const;  // 返回客户端的port

    void onMessage();       // 处理对端发送过来的报文，供Channel回调
    void closeConnection(); // TCP连接断开的回调函数，供Channel回调
    void ErrorConnection(); // TCP连接错误的回调函数，供Channel回调
    void write();           // 处理写事件的回调函数，供Channel回调

    void setCloseConnectionCallback(std::function<void(spConnection)> func);
    void setErrorConnectionCallback(std::function<void(spConnection)> func);
    void setOnMessageCallback(std::function<void(spConnection, std::string &)> func);
    void setSendCompleteCallback(std::function<void(spConnection)> func);

    void send(std::string &message);       // 发送数据，不管在任何线程中，都调用此函数发送数据
    void sendInLoop(std::string &message); // 发送数据，如果当前线程是IO线程，直接调用此函数；如果是工作线程，将此函数传给IO线程

    bool timeout(time_t now, int sec); // 判断TCP连接是否超时（空闲太久）
};
