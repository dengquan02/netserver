// 网络地址协议封装

#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

// socket的地址协议类
class InetAddress
{
private:
    sockaddr_in addr_; // 服务端地址的结构体。

public:
    InetAddress();
    InetAddress(const std::string &ip, uint16_t port); // listenfd
    InetAddress(const sockaddr_in addr);               // clientfd
    ~InetAddress();

    const char *ip() const;       // ip地址（字符串）
    uint16_t port() const;        // 端口（整数）
    const sockaddr *addr() const; // ip地址（sockaddr *）
    void setAddr(sockaddr_in client_addr);
};
