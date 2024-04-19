// 网络通讯的客户端程序。
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <string>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage:./client ip port\n");
        printf("example:./client 172.20.146.222 5005\n\n");
        return -1;
    }

    int sock_fd;
    struct sockaddr_in server_addr;
    char buf[1024];

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("socket() failed.\n");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
    {
        printf("connect(%s:%s) failed.\n", argv[1], argv[2]);
        close(sock_fd);
        return -1;
    }

    printf("connect ok.\n");
    
    printf("开始时间：%ld\n",time(0));

    // 1. 没有分隔符
    // for (int i = 0; i < 10; i++)
    // {
    //     memset(buf, 0, sizeof(buf));
    //     sprintf(buf, "这是第%d个超级女生。", i);

    //     send(sock_fd, buf, strlen(buf), 0);
        
    //     memset(buf, 0, sizeof(buf)); 
    //     recv(sock_fd, buf, sizeof(buf), 0); // 读取报文内容。
    //     printf("recv:%s\n", buf);

    //     sleep(1);
    // }

    // 2. 带四字节的报文头部
    for (int i = 0; i < 100000; i++)
    {
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "这是第%d个超级女生。", i);

        char tmpbuf[1024];
        memset(tmpbuf, 0, sizeof(tmpbuf));
        int len = strlen(buf);
        memcpy(tmpbuf, &len, 4);
        memcpy(tmpbuf + 4, buf, len);

        send(sock_fd, tmpbuf, len + 4, 0);
        

        recv(sock_fd, &len, 4, 0); // 先读取4字节的报文头部。
        memset(buf, 0, sizeof(buf)); 
        recv(sock_fd, buf, len, 0); // 读取报文内容。
        // printf("recv:%s\n", buf);

        // sleep(1);
    }

    // 3. "\r\n\r\n"

    printf("结束时间：%ld\n",time(0));
}