#include "Connection.h"

Connection::Connection(EventLoop *loop, std::unique_ptr<Socket> client_sock, int sep)
    : loop_(loop), connect_sock_(std::move(client_sock)), connect_channel_(new Channel(loop_, connect_sock_->fd())),
      disconnect_(false), input_buffer_(new Buffer(sep)), output_buffer_(new Buffer(sep))
{
    connect_channel_->setReadCallback(std::bind(&Connection::onMessage, this));
    connect_channel_->setCloseCallback(std::bind(&Connection::closeConnection, this));
    connect_channel_->setErrorCallback(std::bind(&Connection::ErrorConnection, this));
    connect_channel_->setWriteCallback(std::bind(&Connection::write, this));
    connect_channel_->useEt();         // 采用边缘触发
    connect_channel_->enableReading(); // 监听读事件
}

Connection::~Connection()
{
}

int Connection::fd() const
{
    return connect_sock_->fd();
}
std::string Connection::ip() const
{
    return connect_sock_->ip();
}
uint16_t Connection::port() const
{
    return connect_sock_->port();
}

// 处理对端发送过来的报文，供Channel回调
void Connection::onMessage()
{
    // 将TCP接收缓冲区的数据拷贝到自定义的接收缓冲区input_buffer_
    // 当TCP接收缓冲区的数据已经全部读取后，从input_buffer_中提取一个完整的用户message，交由业务逻辑处理
    char buffer[1024];
    while (true) // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
    {
        bzero(&buffer, sizeof(buffer));
        ssize_t nread = read(fd(), buffer, sizeof(buffer)); 
        if (nread > 0)                                      // 成功的读取到了数据。
        {
            input_buffer_->append(buffer, nread); // 把读取的数据追加到接收缓冲区中
        }
        else if (nread == -1 && errno == EINTR) // 读取数据的时候被信号中断，继续读取。（非必须）
        {
            continue;
        }
        else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕。
        {
            std::string message;
            while (true)
            {
                if (input_buffer_->pickMessage(message) == false)
                    break;
                // printf("message (eventfd=%d): %s\n", Fd(), message.c_str());
                lastatime_ = Timestamp::now();
                on_message_callback_(shared_from_this(), message); // 回调TcpServer::OnMessage
            }
            break;
        }
        else if (nread == 0) // 客户端连接已断开。
        {
            closeConnection();
            break;
        }
    }
}

// TCP连接断开的回调函数，供Channel回调
void Connection::closeConnection()
{
    disconnect_ = true;
    connect_channel_->remove();                     // 从事件循环中删除Channel
    close_connection_callback_(shared_from_this()); // 回调TcpServer::closeConnection
}
// TCP连接错误的回调函数，供Channel回调
void Connection::ErrorConnection()
{
    disconnect_ = true;
    connect_channel_->remove();
    error_connection_callback_(shared_from_this());
}

void Connection::setCloseConnectionCallback(std::function<void(spConnection)> func)
{
    close_connection_callback_ = func;
}
void Connection::setErrorConnectionCallback(std::function<void(spConnection)> func)
{
    error_connection_callback_ = func;
}
void Connection::setOnMessageCallback(std::function<void(spConnection, std::string &)> func)
{
    on_message_callback_ = func;
}
void Connection::setSendCompleteCallback(std::function<void(spConnection)> func)
{
    send_complete_callback_ = func;
}

// 发送数据，不管在任何线程中，都调用此函数发送数据
void Connection::send(std::string &message)
{
    if (disconnect_)
    {
        printf("连接已经断开，Send直接返回\n");
        return;
    }

    if (loop_->isInLoopThread())
    {
        // 当前线程是事件循环线程（IO线程），直接调用SendInLoop发送数据
        sendInLoop(message);
    }
    else
    {
        // 当前线程不是事件循环线程（IO线程），把SendInLoop交给IO线程去执行
        loop_->queueInLoop(std::bind(&Connection::sendInLoop, this, message)); // 实际上传进message的副本
    }
}

// 发送数据，如果当前线程是IO线程，直接调用此函数；如果是工作线程，将此函数传给IO线程
void Connection::sendInLoop(std::string &message)
{
    output_buffer_->appendWithSep(message.data(), message.size()); // 需要发送的数据保存在Connection的发送缓冲区中
    connect_channel_->enableWriting();                             // 注册写事件
}

// 处理写事件的回调函数，供Channel回调
void Connection::write()
{
    // 尝试把缓冲区数据全部发送出去
    int writen = ::send(fd(), output_buffer_->data(), output_buffer_->size(), 0);
    // 从缓冲区删除已经成功发送的字节数
    if (writen > 0)
        output_buffer_->erase(0, writen);

    // 如果缓冲区没有数据了，说明已全部发送（到TCP缓冲区中去了）完毕，停止关注写事件
    if (output_buffer_->size() == 0)
    {
        connect_channel_->disableWriting();
        send_complete_callback_(shared_from_this());
    }
}

// 判断TCP连接是否超时（空闲太久）
bool Connection::timeout(time_t now, int sec)
{
    return now - lastatime_.toInt() > sec;
}
