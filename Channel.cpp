#include "Channel.h"

Channel::Channel(EventLoop *loop, int fd) : loop_(loop), fd_(fd) {}
Channel::~Channel() {} // 析构函数中，不要销毁ep_和fd_，Channel类需要他们而不拥有他们

int Channel::fd()
{
    return fd_;
}
void Channel::useEt() // 采用边缘触发
{
    events_ |= EPOLLET;
}

void Channel::enableReading() // 注册读事件
{
    events_ |= EPOLLIN;
    loop_->updateChannel(this);
}
void Channel::disableReading() // 取消注册读事件
{
    events_ &= ~EPOLLIN;
    loop_->updateChannel(this);
}
void Channel::enableWriting() // 注册写事件
{
    events_ |= EPOLLOUT;
    loop_->updateChannel(this);
}
void Channel::disableWriting() // 取消注册写事件
{
    events_ &= ~EPOLLOUT;
    loop_->updateChannel(this);
}
void Channel::disableAll() // 取消监视全部事件
{
    events_ = 0;
    loop_->updateChannel(this);
}

void Channel::remove() // 从事件循环中删除Channel
{
    disableAll(); // 可以不写。既然要删除，关不关注也不重要了
    loop_->removeChannel(this);
}

void Channel::setInEpoll() // 把in_epoll_设置为true
{
    in_epoll_ = true;
}
void Channel::setRevents(uint32_t ev) // 设置revents的值为ev
{
    revents_ = ev;
}
bool Channel::inEpoll()
{
    return in_epoll_;
}
uint32_t Channel::events()
{
    return events_;
}
uint32_t Channel::revents()
{
    return revents_;
}

// 事件处理函数：epoll_wait()返回时执行它
void Channel::handleEvent()
{
    if (revents_ & EPOLLRDHUP) // 对方已关闭，如果有些系统检测不到，可以使用EPOLLIN，recv()返回0。（非必须）
    {
        close_callback_(); // 回调Connection::CloseConnection
    }
    else if (revents_ & (EPOLLIN | EPOLLPRI)) // 接收缓冲区中有数据可以读。
    {
        read_callback_(); // 如果是accept_channel，回调Acceptor::NewConnection；如果是connect_channel，回调Connection::OnMessage
    }
    else if (revents_ & EPOLLOUT) // 有数据需要写
    {
        write_callback_(); // 回调Connection::Write
    }
    else // 其它事件，都视为错误。
    {
        error_callback_(); // 回调Connection::ErrorConnection
    }
}

// 设置fd_读事件的回调函数
void Channel::setReadCallback(std::function<void()> func)
{
    read_callback_ = func;
}

void Channel::setCloseCallback(std::function<void()> func)
{
    close_callback_ = func;
}
void Channel::setErrorCallback(std::function<void()> func)
{
    error_callback_ = func;
}
void Channel::setWriteCallback(std::function<void()> func)
{
    write_callback_ = func;
}
