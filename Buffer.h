#pragma once

#include <string>
#include <iostream>
#include <string.h>

class Buffer
{
private:
    std::string buf_;    // 用于存放数据
    const uint16_t sep_; // 0-无分隔符（固定长度、视频会议） 1-四字节的报文头部 2-"\r\n\r\n分隔符"（http协议）

public:
    Buffer(uint16_t sep = 0);
    ~Buffer();

    void append(const char *data, size_t size); // 数据追加到buf_中
    void appendWithSep(const char *data, size_t size); // 数据追加到buf_中，附加报文分隔符
    void erase(size_t pos, size_t n);                  // 从buf_的pos开始，删除n个字节
    size_t size();                                     // 返回buf_大小
    const char *data();                                // 返回buf_的首地址
    void clear();                                      // 清空buf
    bool pickMessage(std::string &s);                  // 从buf_中拆分出一个报文，存放在s中，如果buf_中没有报文，返回false
};
