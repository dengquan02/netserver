/*
 * 程序名：echo_server.cpp，此程序用于演示采用epoll模型实现网络通讯的服务端。
 */
#include <signal.h>
#include "EchoServer.h"

EchoServer *echo_server;

// 信号2和15的信号处理函数
void Stop(int sig)
{
    printf("sig=%d\n", sig);

    echo_server->Stop();

    printf("echo_server已停止！\n");
    delete echo_server;
    // 如果 EchoServer 在它的生命周期中使用了某种需等待系统回收的资源（如系统锁、进程、线程等），
    // 那么在 delete 语句执行时可能需要等待这些资源的回收，从而导致阻塞。
    printf("delete echo_server\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage: ./echo_server ip port\n");
        printf("example: ./echo_server 172.20.146.222 5005\n\n");
        return -1;
    }

    signal(SIGTERM, Stop);
    signal(SIGINT, Stop);

    echo_server = new EchoServer(argv[1], atoi(argv[2]), 1, 10, 0);
    echo_server->Start();
    
    return 0;
}