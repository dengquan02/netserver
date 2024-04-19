#pragma once
#include "EventLoop.h"
#include "Socket.h"
#include "Channel.h"
#include "Acceptor.h"
#include "Connection.h"
#include "ThreadPool.h"
#include <map>
#include <mutex>

class TcpServer
{
private:
    // 堆内存和栈内存都可以，考虑到从事件循环需要堆内存，因此统一使用堆内存
    std::unique_ptr<EventLoop> main_loop_;                                  // 主事件循环
    std::vector<std::unique_ptr<EventLoop>> sub_loops_;                     // 存放从事件的容器
    Acceptor acceptor_;                                                     // 一个TcpServer只创建一个Acceptor对象
    int thread_num_;                                                        // 线程池的大小
    ThreadPool thread_pool_;                                                // 线程池
    std::mutex conns_mutex_;                                                // 保护conns_的互斥锁
    std::map<int, spConnection> conns_;                                     // 一个TcpServer可以有多个Connection对象，存放在map容器中
    std::function<void(spConnection)> new_connection_cb_;                   // 回调EchoServer::HandleNewConnection()。
    std::function<void(spConnection)> close_connection_cb_;                 // 回调EchoServer::HandleClose()。
    std::function<void(spConnection)> error_connection_cb_;                 // 回调EchoServer::HandleError()。
    std::function<void(spConnection, std::string &message)> on_message_cb_; // 回调EchoServer::HandleOnMessage()。
    std::function<void(spConnection)> send_complete_cb_;                    // 回调EchoServer::HandleSendComplete()。
    std::function<void(EventLoop *)> epoll_timeout_cb_;                     // 回调EchoServer::HandleTimeOut()。
    int sep_;

public:
    TcpServer(const std::string &ip, const uint16_t port, int sep, int thread_num = 3);
    ~TcpServer();

    void start();
    void stop(); // 停止IO线程和事件循环

    void newConnection(std::unique_ptr<Socket> client_sock); // 处理新的客户端连接
    void closeConnection(spConnection conn);                 // 关闭客户端连接，在Connection类中回调该函数
    void closeConnectionInLoop(spConnection conn);
    void errorConnection(spConnection conn);                 // 客户端连接错误，在Connection类中回调该函数
    void errorConnectionInLoop(spConnection conn);
    void onMessage(spConnection conn, std::string &message); // 处理客户端的请求报文，在Connection类中回调此函数
    void sendComplete(spConnection conn);                    // 数据发送完毕后，在Connection中回调该函数
    void epollTimeout(EventLoop *loop);                      // epoll_wait()超时，在EventLoop中回调该函数

    void setNewConnectionCb(std::function<void(spConnection)> func);
    void setCloseConnectionCb(std::function<void(spConnection)> func);
    void setErrorConnectionCb(std::function<void(spConnection)> func);
    void setOnMessageCb(std::function<void(spConnection, std::string &message)> func);
    void setSendCompleteCb(std::function<void(spConnection)> func);
    void setEpollTimeoutCb(std::function<void(EventLoop *)> func);

    void removeConn(int fd); // 删除conns_中的Connection对象，在EventLoop::HandleTimer中回调该函数
};
