#include "Socket.h"

// 创建一个非阻塞的socket
int createNonBlocking()
{
    // 创建服务端用于监听的listenfd。 SOCK_NONBLOCK设置非阻塞的IO。
    int listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (listen_fd < 0)
    {
        printf("%s:%s:%d listen socket create error: %d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        exit(-1);
    }
    return listen_fd;
}

Socket::Socket(int fd) : fd_(fd) {}

Socket::~Socket()
{
    ::close(fd_);
}

int Socket::fd() const
{
    return fd_;
}

std::string Socket::ip() const
{
    return ip_;
}
uint16_t Socket::port() const
{
    return port_;
}

void Socket::setIpPort(const std::string &ip, uint16_t port)
{
    ip_ = ip;
    port_ = port;
}


void Socket::setReuseaddr(bool on)
{
    // 打开地址复用功能：
    // 如果当前启动进程绑定的 IP+PORT 与处于TIME_WAIT状态的连接占用的 IP+PORT 存在冲突，但是新启动的进程使用了 SO_REUSEADDR 选项，那么该进程就可以绑定成功
    // 绑定的 IP地址 + 端口时，只要 IP 地址不是正好(exactly)相同（例如，0.0.0.0:8888和192.168.1.100:8888），那么允许绑定
    int opt = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, static_cast<socklen_t>(sizeof opt));
}

void Socket::setReuseport(bool on)
{
    // 不使用Nagle算法
    // Nagle 算法默认是打开的，如果对于一些需要小数据包交互的场景的程序，比如，telnet 或 ssh 这样的交互性比较强的程序，则需要关闭 Nagle 算法
    int opt = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, TCP_NODELAY, &opt, static_cast<socklen_t>(sizeof opt)); // 必须的。
}

void Socket::setTcpnodelay(bool on)
{
    // 允许多个进程绑定相同的 IP 地址和端口
    int opt = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &opt, static_cast<socklen_t>(sizeof opt)); // 有用，但是，在Reactor中意义不大。
}

void Socket::setKeepalive(bool on)
{
    // 启用TCP保活
    int opt = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &opt, static_cast<socklen_t>(sizeof opt)); // 可能有用，但是，建议自己做心跳。
}

void Socket::bind(const InetAddress &server_addr)
{
    if (::bind(fd_, server_addr.addr(), sizeof(sockaddr)) < 0)
    {
        perror("bind() failed");
        close(fd_);
        exit(-1);
    }
    setIpPort(server_addr.ip(), server_addr.port());
}

void Socket::listen(int backlog)
{
    // TCP 全连接队列的最大值取决于 somaxconn 和 backlog 之间的最小值
    if (::listen(fd_, backlog) != 0) // 在高并发的网络服务器中，第二个参数backlog要大一些。
    {
        perror("listen() failed");
        close(fd_);
        exit(-1);
    }
}

int Socket::accept(InetAddress &client_addr)
{
    sockaddr_in peer_addr;
    socklen_t len = sizeof(peer_addr);
    int client_fd = accept4(fd_, (sockaddr *)&peer_addr, &len, SOCK_NONBLOCK);

    client_addr.setAddr(peer_addr);

    return client_fd;
    
}
