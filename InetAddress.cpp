#include "InetAddress.h"

InetAddress::InetAddress() {}

InetAddress::InetAddress(const std::string &ip, uint16_t port)
{
    addr_.sin_family = AF_INET;                    // IPv4网络协议的套接字类型。
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); // 服务端用于监听的ip地址。
    addr_.sin_port = htons(port);                  // 服务端用于监听的端口。
}

InetAddress::InetAddress(const sockaddr_in addr) : addr_(addr) {} // clientfd

InetAddress::~InetAddress() {}

const char *InetAddress::ip() const
{
    return inet_ntoa(addr_.sin_addr);
}

uint16_t InetAddress::port() const
{
    return ntohs(addr_.sin_port);
}

const sockaddr *InetAddress::addr() const
{
    return (sockaddr *)&addr_;
}

void InetAddress::setAddr(sockaddr_in client_addr)
{
    addr_ = client_addr;
}