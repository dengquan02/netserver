#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "InetAddress.h"

// 创建一个非阻塞的socket
int createNonBlocking();

class Socket
{
private:
    const int fd_;
    std::string ip_; // 如果是监听socket，存放服务端ip；如果是已连接socket，存放对端（客户端）ip
    uint16_t port_;

public:
    Socket(int fd);
    ~Socket();

    int fd() const; // 返回fd_成员
    std::string ip() const;
    uint16_t port() const;
    void setIpPort(const std::string &ip, uint16_t port);
    
    /* 调用setsockopt设置一些socket选项 */
    void setReuseaddr(bool on); // 设置SO_REUSEADDR选项，true-打开，false-关闭
    void setReuseport(bool on);
    void setTcpnodelay(bool on);
    void setKeepalive(bool on);

    void bind(const InetAddress &server_addr); // 绑定服务器地址
    void listen(int backlog = 128);            // 开启监听
    int accept(InetAddress &client_addr);      // 接受新客户端连接请求（从全连接队列中取出socket）
};