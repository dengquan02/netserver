#include "Buffer.h"

Buffer::Buffer(uint16_t sep) : sep_(sep)
{
}

Buffer::~Buffer()
{
}

// 数据追加到buf_中
void Buffer::append(const char *data, size_t size)
{
    buf_.append(data, size);
}

// 数据追加到buf_中
void Buffer::appendWithSep(const char *data, size_t size)
{
    if (sep_ == 0)
        buf_.append(data, size); // 处理报文内容
    else if (sep_ == 1)
    {
        buf_.append((char *)&size, 4); // 处理报文长度（报头）
        buf_.append(data, size);       // 处理报文内容
    }
    else
    {
        buf_.append(data, size); // 处理报文内容
        buf_.append("\r\n\r\n");
    }
}

// 从buf_的pos开始，删除n个字节
void Buffer::erase(size_t pos, size_t n)
{
    buf_.erase(pos, n);
}

// 返回buf_大小
size_t Buffer::size()
{
    return buf_.size();
}
// 返回buf_的首地址
const char *Buffer::data()
{
    return buf_.data();
}
// 清空buf
void Buffer::clear()
{
    buf_.clear();
}

// 从buf_中拆分出一个报文，存放在s中，如果buf_中没有报文，返回false
bool Buffer::pickMessage(std::string &s)
{
    if (buf_.empty())
        return false;

    if (sep_ == 0)
    {
        s = buf_;
        buf_.clear();
    }
    else if (sep_ == 1)
    {
        int len = 0;
        memcpy(&len, data(), 4); // 获取报文头部

        if (size() < len + 4) // 报文内容不完整
            return false;

        s = buf_.substr(4, len); 

        erase(0, len + 4);
    }
    else
    {
        
    }

    return true;
}
