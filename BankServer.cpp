#include "BankServer.h"

BankServer::BankServer(const std::string &ip, const uint16_t port, int sep, int sub_thread_num, int work_thread_num)
    : tcpserver_(ip, port, sep, sub_thread_num), work_thread_pool_(work_thread_num, "WORKS")
{
    // 以下代码非必须，业务关心什么事件，就指定相应的回调函数
    tcpserver_.setNewConnectionCb(std::bind(&BankServer::HandleNewConnection, this, std::placeholders::_1));
    tcpserver_.setCloseConnectionCb(std::bind(&BankServer::HandleCloseConnection, this, std::placeholders::_1));
    tcpserver_.setErrorConnectionCb(std::bind(&BankServer::HandleErrorConnection, this, std::placeholders::_1));
    tcpserver_.setOnMessageCb(std::bind(&BankServer::HandleOnMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcpserver_.setSendCompleteCb(std::bind(&BankServer::HandleSendComplete, this, std::placeholders::_1));
    // tcpserver_.SetEpollTimeoutCb(std::bind(&BankServer::HandleEpollTimeout, this, std::placeholders::_1));
}

BankServer::~BankServer()
{
}

// 启动服务
void BankServer::Start()
{
    tcpserver_.start();
}

// 停止服务
void BankServer::Stop()
{
    // 停止工作线程
    work_thread_pool_.stop();
    printf("工作线程已经停止\n");

    // 停止IO线程和事件循环
    tcpserver_.stop();
}


// 处理新的客户端连接，在TcpServer类中回调该函数
void BankServer::HandleNewConnection(spConnection conn)
{
    // std::cout << "New Connection Come in." << std::endl;
    printf("%s new connection(fd=%d, ip=%s, port=%d) ok.\n", Timestamp::now().toString().c_str(), conn->fd(), conn->ip().c_str(), conn->port());

    // 根据业务的需求，在这里可以增加其它的代码。
}

// 关闭客户端连接，在TcpServer类中回调该函数
void BankServer::HandleCloseConnection(spConnection conn)
{
    // std::cout << "BankServer conn closed." << std::endl;
    printf("%s connection closed(fd=%d, ip=%s, port=%d) ok.\n", Timestamp::now().toString().c_str(), conn->fd(), conn->ip().c_str(), conn->port());

    // 根据业务的需求，在这里可以增加其它的代码。
}

// 客户端连接错误，在TcpServer类中回调该函数
void BankServer::HandleErrorConnection(spConnection conn)
{
    // std::cout << "BankServer conn error." << std::endl;

    // 根据业务的需求，在这里可以增加其它的代码。
}

// 处理客户端的请求报文，在TcpServer类中回调此函数
void BankServer::HandleOnMessage(spConnection conn, std::string &message)
{
    // printf("BankServer::HandleOnMessage thread is %ld.\n", syscall(SYS_gettid));

    if (work_thread_pool_.size() == 0)
    {
        // 如果没有工作线程，表示在IO线程中进行计算
        OnMessage(conn, message);
    }
    else
    {
        // 把业务添加到工作线程池的队列中
        work_thread_pool_.addTask(std::bind(&BankServer::OnMessage, this, conn, message)); // 这里传进去的message是一个副本（拷贝）
        // work_thread_pool_.AddTask(std::bind(&BankServer::OnMessage, this, conn, ref(message))); // 这里传进去的message才是引用
    }
}

// 处理客户端的请求报文，用于添加给线程池
void BankServer::OnMessage(spConnection conn, std::string &message)
{
    // printf("%s message (eventfd=%d): %s\n", Timestamp::now().tostring().c_str(), conn->Fd(), message.c_str());

    // 在这里经过若干步骤的运算
    message = "reply:" + message; // 回显业务

    // conn->Send(message.data(), message.size());
    conn->send(message);
}


// 数据发送完毕后，在TcpServer中回调该函数
void BankServer::HandleSendComplete(spConnection conn)
{
    // std::cout << "Message send complete." << std::endl;

    // 根据业务的需求，在这里可以增加其它的代码。
}

// epoll_wait()超时，在TcpServer中回调该函数
void BankServer::HandleEpollTimeout(EventLoop *loop)
{
    // std::cout << "BankServer timeout." << std::endl;

    // 根据业务的需求，在这里可以增加其它的代码。
}