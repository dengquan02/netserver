#pragma once
#include "TcpServer.h"
#include "EventLoop.h"
#include "Connection.h"
#include "ThreadPool.h"

/*
 BankServer类：回显服务器
*/

class BankServer
{
private:
    TcpServer tcpserver_;
    ThreadPool work_thread_pool_; // 工作线程池

public:
    BankServer(const std::string &ip, const uint16_t port, int sep, int sub_thread_num = 3, int work_thread_num = 5);
    ~BankServer();

    void Start(); // 启动服务
    void Stop(); // 停止服务

    void HandleNewConnection(spConnection conn);                   // 处理新的客户端连接
    void HandleCloseConnection(spConnection conn);                 // 关闭客户端连接，在TcpServer类中回调该函数
    void HandleErrorConnection(spConnection conn);                 // 客户端连接错误，在TcpServer类中回调该函数
    void HandleOnMessage(spConnection conn, std::string &message); // 处理客户端的请求报文，在TcpServer类中回调此函数
    void HandleSendComplete(spConnection conn);                    // 数据发送完毕后，在TcpServer中回调该函数
    void HandleEpollTimeout(EventLoop *loop);                     // epoll_wait()超时，在TcpServer中回调该函数

    void OnMessage(spConnection conn, std::string &message); // 处理客户端的请求报文，用于添加给线程池
};