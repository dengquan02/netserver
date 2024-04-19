#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, const uint16_t port, int sep, int thread_num)
    : thread_num_(thread_num), main_loop_(new EventLoop(true)), acceptor_(main_loop_.get(), ip, port),
      thread_pool_(thread_num_, "IO"), sep_(sep)
{
    main_loop_->setEpollWaitTimeoutCallback(std::bind(&TcpServer::epollTimeout, this, std::placeholders::_1));

    acceptor_.setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1));

    // 创建从事件循环
    for (int i = 0; i < thread_num_; ++i)
    {
        sub_loops_.emplace_back(new EventLoop(false, 5, 10));
        sub_loops_[i]->setEpollWaitTimeoutCallback(std::bind(&TcpServer::epollTimeout, this, std::placeholders::_1));
        sub_loops_[i]->setRemoveConnCallback(std::bind(&TcpServer::removeConn, this, std::placeholders::_1));
        thread_pool_.addTask(std::bind(&EventLoop::run, sub_loops_[i].get())); // 在线程池中运行从事件循环
    }
}

TcpServer::~TcpServer()
{
}

void TcpServer::start()
{
    main_loop_->run();
}

// 停止IO线程和事件循环
void TcpServer::stop()
{
    // 停止主事件循环
    main_loop_->stop();
    printf("主事件循环已停止！\n");

    // 停止从事件循环
    for (auto &x : sub_loops_)
    {
        x->stop();
    }
    printf("从事件循环已停止！\n");

    // 停止IO线程
    thread_pool_.stop();
    printf("IO线程已停止！\n");
}

// 处理新的客户端连接
void TcpServer::newConnection(std::unique_ptr<Socket> client_sock)
{
    // 把新建的conn分配给从事件循环
    int loop_index = client_sock->fd() % thread_num_; // 简单的分配算法
    spConnection conn(new Connection(sub_loops_[loop_index].get(), std::move(client_sock), sep_));

    conn->setCloseConnectionCallback(std::bind(&TcpServer::closeConnection, this, std::placeholders::_1));
    conn->setErrorConnectionCallback(std::bind(&TcpServer::errorConnection, this, std::placeholders::_1));
    conn->setOnMessageCallback(std::bind(&TcpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    conn->setSendCompleteCallback(std::bind(&TcpServer::sendComplete, this, std::placeholders::_1));

    {
        std::lock_guard<std::mutex> gd(conns_mutex_);
        conns_[conn->fd()] = conn; // conn存放在TcpServer的map容器
    }
    sub_loops_[loop_index]->newConnection(conn); // conn存放在对应事件循环的map容器

    new_connection_cb_(conn); // 回调EchoServer::HandleNewConnection()。
}

// 关闭客户端连接，在Connection类中回调该函数
void TcpServer::closeConnection(spConnection conn)
{
    // MainEventLoop中执行closeConnectionInLoop
    main_loop_->queueInLoop(std::bind(&TcpServer::closeConnectionInLoop, this, conn));
}
void TcpServer::closeConnectionInLoop(spConnection conn)
{
    if (close_connection_cb_)
        close_connection_cb_(conn); // 回调EchoServer::HandleCloseConnection

    {
        std::lock_guard<std::mutex> gd(conns_mutex_);
        conns_.erase(conn->fd());
    }
}

// 客户端连接错误，在Connection类中回调该函数
void TcpServer::errorConnection(spConnection conn)
{
    // MainEventLoop中执行errorConnectionInLoop
    main_loop_->queueInLoop(std::bind(&TcpServer::errorConnectionInLoop, this, conn));
}
void TcpServer::errorConnectionInLoop(spConnection conn)
{
    if (error_connection_cb_)
        error_connection_cb_(conn); // 回调EchoServer::HandleErrorConnection

    {
        std::lock_guard<std::mutex> gd(conns_mutex_);
        conns_.erase(conn->fd());
    }
}

// 处理客户端的请求报文，在Connection类中回调此函数
void TcpServer::onMessage(spConnection conn, std::string &message)
{
    if (on_message_cb_)
        on_message_cb_(conn, message); // 回调EchoServer::HandleOnMessage
}

// 数据发送完毕后，在Connection中回调该函数
void TcpServer::sendComplete(spConnection conn)
{
    if (send_complete_cb_)
        send_complete_cb_(conn);
}

// epoll_wait()超时，在EventLoop中回调该函数
void TcpServer::epollTimeout(EventLoop *loop)
{
    if (epoll_timeout_cb_)
        epoll_timeout_cb_(loop);
}

void TcpServer::setNewConnectionCb(std::function<void(spConnection)> func)
{
    new_connection_cb_ = func;
}
void TcpServer::setCloseConnectionCb(std::function<void(spConnection)> func)
{
    close_connection_cb_ = func;
}
void TcpServer::setErrorConnectionCb(std::function<void(spConnection)> func)
{
    error_connection_cb_ = func;
}
void TcpServer::setOnMessageCb(std::function<void(spConnection, std::string &message)> func)
{
    on_message_cb_ = func;
}
void TcpServer::setSendCompleteCb(std::function<void(spConnection)> func)
{
    send_complete_cb_ = func;
}
void TcpServer::setEpollTimeoutCb(std::function<void(EventLoop *)> func)
{
    epoll_timeout_cb_ = func;
}

// 删除conns_中的Connection对象，在EventLoop::HandleTimer中回调该函数
void TcpServer::removeConn(int fd)
{
    {
        std::lock_guard<std::mutex> gd(conns_mutex_);
        conns_.erase(fd);
    }
}
