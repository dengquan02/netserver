#include "Acceptor.h"

Acceptor::Acceptor(EventLoop *loop, const std::string &ip, const uint16_t port)
    : loop_(loop), accept_sock_(createNonBlocking()), accept_channel_(loop_, accept_sock_.fd())
{
    accept_sock_.setReuseaddr(true);
    accept_sock_.setTcpnodelay(true);
    accept_sock_.setReuseport(true);
    accept_sock_.setKeepalive(true);
    InetAddress server_addr(ip, port); // 服务端的地址和协议
    accept_sock_.bind(server_addr);
    accept_sock_.listen();

    accept_channel_.setReadCallback(std::bind(&Acceptor::newConnection, this));
    accept_channel_.enableReading();
}

Acceptor::~Acceptor()
{
}

// 处理新的客户端连接
void Acceptor::newConnection()
{
    InetAddress client_addr; // 客户端的地址和协议
    std::unique_ptr<Socket> client_sock(new Socket(accept_sock_.accept(client_addr)));
    client_sock->setIpPort(client_addr.ip(), client_addr.port());

    // 这里使用回调函数的目的：为了在TcpServer类中创建Connection对象conn，因为conn应该由TcpServer来创建并管理
    new_connection_callback_(std::move(client_sock)); // 回调TcpServer::NewConnection
}

void Acceptor::setNewConnectionCallback(std::function<void(std::unique_ptr<Socket>)> func)
{
    new_connection_callback_ = func;
}