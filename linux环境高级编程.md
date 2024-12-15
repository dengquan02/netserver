

------

# 项目梳理

## Reactor模型

### Multi-Reactor（为什么选用）





## 三大核心模块

### Channel类

### Epoll类

### EventLoop类



## 主线梳理

### 建立连接

### 消息读取

### 消息发送

### 连接断开

#### 连接被动断开

- 在Connection::onMessage中感知到，调用Connection::closeConnection，接着回调TcpServer::closeConnection，此时在SubEventLoop线程中运行。
- closeConnection所做工作是从TcpServer对象中删除当前Connection对象，而TcpServer对象是属于MainEventLoop的，因此应当在MainEventLoop中运行closeConnectionInLoop（使用上面一样的线程切换技术）。这也是贯彻“**One Loop Per Thread**”的理念。

#### 连接主动断开

- 先在 EchoServer::Stop 中， `work_thread_pool_.stop();` 停止工作线程（如果有正在处理中的请求报文，会等待其处理完毕然后停止，但对于位于工作线程池的任务队列中那些未来得及处理的请求报文，当前程序则会忽略），然后执行 `tcpserver_.stop();`
- 在 TcpServer::stop 中，依次：
  - `main_loop_->stop();` 停止主事件循环
  - 停止从事件循环
  - `thread_pool_.stop();` 停止IO线程（如果有正在发送中的响应报文，会等待其发送完毕然后停止，但对于位于IO线程池的任务队列中那些未来得及发送的响应报文，当前程序则会忽略）

> **如果Connection中有正在发送的数据，怎么保证在触发Connection关闭机制后，能先把数据发送完再释放Connection对象的资源？**
>
> 我的解决方法：修改线程池运行逻辑
>
> ```cpp
> /* 修改前 */
> // 线程运行逻辑：执行完当前任务后，只要stop_为true就会退出
> while (!stop_)
> {
>     ......
>     task(); // 执行任务
> }
> 
> 
> /* 修改后 */
> // 线程运行逻辑：执行完当前任务后，只有stop_为true且task_queue_为空才会退出
> while (!stop_ || !task_queue_.empty())
> {
>     ......
>     task(); // 执行任务
> }
> void ThreadPool::addTask(std::function<void()> task)
> {
>     // 如果不添加下面这行代码，表示可以源源不断地调用addTask填充task_queue_，会导致线程一直无法退出
>     if (stop_) return; // 如果已经调用析构函数了，那么就不能继续添加任务了
>     ......
> }
> ```
>
> 另一种设计：
>
> 

​	

## 线程篇 —  One Loop Per Thread

One Loop Per Thread的含义就是，一个EventLoop和一个线程唯一绑定，和这个EventLoop有关的，被这个EventLoop管辖的一切操作都必须在这个EventLoop绑定线程中执行。

但有一种例外，比如SubEventLoop的创建和释放在MainEventLoop中由TcpServer来管理，





------



# Linux环境高级编程

## 静态库和动态库

查看代码结构：

```shell
dq@LAPTOP-RMIONVTK:~/reactor$ tree
.
├── app
│   └── demo01.cpp
└── tools
    ├── public.cpp
    └── public.h

2 directories, 3 files
```

### 静态库

**制作方法1**（csapp）

```shell
dq@LAPTOP-RMIONVTK:~/reactor/tools$ g++ -c public.cpp
dq@LAPTOP-RMIONVTK:~/reactor/tools$ ar rcs libpublic.a public.o
```

制作方法2（码农论坛-吴）

```shell
dq@LAPTOP-RMIONVTK:~/reactor/tools$ g++ -c -o libpublic_.a public.cpp
```

实际上，方法2制作的 libpublic_.a 是 public.o 的重命名而已。

```shell
dq@LAPTOP-RMIONVTK:~/reactor/tools$ ll
total 28
drwxr-xr-x 2 dq dq 4096 Dec 15 19:21 ./
drwxr-xr-x 4 dq dq 4096 Dec 15 18:49 ../
-rw-r--r-- 1 dq dq 3108 Dec 15 19:21 libpublic.a
-rw-r--r-- 1 dq dq 2944 Dec 15 19:21 libpublic_.a
-rw-r--r-- 1 dq dq  133 Dec 15 18:59 public.cpp
-rw-r--r-- 1 dq dq   76 Dec 15 19:02 public.h
-rw-r--r-- 1 dq dq 2944 Dec 15 19:21 public.o
dq@LAPTOP-RMIONVTK:~/reactor/tools$ diff libpublic_.a public.o -s
Files libpublic_.a and public.o are identical
```

**使用**：-L 指定路径，-l 指定库名

```shell
dq@LAPTOP-RMIONVTK:~/reactor/app$ g++ -o demo01 demo01.cpp -L/home/dq/reactor/tools/ -lpublic
```

------

### 动态库

**制作**

```shell
dq@LAPTOP-RMIONVTK:~/reactor/tools$ g++ -fpic -shared -o libpublic.so public.cpp
```

**使用**

不规范：（csapp）

```shell
dq@LAPTOP-RMIONVTK:~/reactor/app$ g++ -o demo01 demo01.cpp ../tools/libpublic
.so
```

规范：（吴）

运行可执行程序时，需要提前设置 LD_LIBRARY_PATH 环境变量。

如果动态库和静态库同时存在，编译器将优先使用动态库。

```shell
dq@LAPTOP-RMIONVTK:~/reactor/app$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/dq/reactor/tools
dq@LAPTOP-RMIONVTK:~/reactor/app$ echo $LD_LIBRARY_PATH
:/home/dq/reactor/tools
dq@LAPTOP-RMIONVTK:~/reactor/app$ g++ -o demo01 demo01.cpp -L../tools -lpublic
```

**动态库的概念**

程序在编译时不会把库文件的二进制代码链接到目标程序中，而是在运行时候才被载入。
如果多个程序中用到了同一动态库中的函数，那么在内存中只有一份，避免了空间浪费问题。

**动态库的特点**

- 程序运行在运行的过程中，需要用到动态库的时候才把动态库的二进制代码载入内存。
- 可以实现进程之间的代码共享，因此动态库也称为共享库。
- **程序升级比较简单，不需要重新编译程序，只需要更新动态库就行了。**

## 高性能网络模式：Reactor 和 Proactor

### Reactor

#### 单 Reactor 单进/线程

#### 单 Reactor 多进/线程

#### 多 Reactor 多进/线程

### Proactor



## I/O多路复用

多进/线程的网络服务端：

- 为每个客户端连接创建一个进/线程，消耗的资源很多
- 1核2GB的虚拟机，大概可以创建一百多个进/线程

IO多路复用：

- 用一个IO多路复用处理多个TCP连接，减少系统开销
- 三种模型：select(1024)、poll(数千)、epoll(百万)是内核提供给用户态的多路复用系统调用，**进程可以通过一个系统调用函数从内核中获取多个事件**。

网络通讯-**读事件**：

1. 已连接队列中有已经准备好的socket（有新的client连上来）
2. 接收缓存中有数据可以读（对端发送的报文已经到达）
3. tcp连接已断开（对端调用close()函数关闭了连接）

网络通讯-**写事件**：

发送缓冲区未满，可以写入数据（可以向对端发送报文）

### select

select 实现多路复用的方式是，将已连接的 Socket 都放到一个**文件描述符集合**，然后调用 select 函数将文件描述符集合**拷贝**到内核里，让内核通过**遍历**文件描述符集合的方式来检查是否有网络事件产生，将检查到有事件产生的 Socket 标记为可读或可写， 接着再把整个文件描述符集合**拷贝**回用户态里，然后用户态还需要再通过**遍历**的方法找到可读或可写的 Socket，然后再对其处理。

**存在的问题：**

- 2 次「遍历」：采用轮询方式扫描bitmap，性能会随着socket数量增多而下降
- 2 次「拷贝」：每次调用 select() 需要拷贝bitmap两次
-  bitmap的大小（单个进/线程打开的socket数量）由FD_SETSIZE宏设置，默认是 1024 个，可以修改增大，但效率将更低。

### poll

poll模型在select模型的基础之上做了一点点改变，但都是使用「线性结构」来存储进程关注的 Socket 集合，没有从根本上解决问题。

**存在的问题：**

- poll使用结构体数组来存储所关注的文件描述符，用链表形式组织。
- 每调用一次select()需要拷贝两次bitmap，poll()拷贝一次结构体数组
- poll 突破了 select 的文件描述符个数限制（当然还会受到系统文件描述符限制），但也是遍历的方法，监视的socket越多，效率越低。

### epoll

1. epoll 在内核里使用**红黑树（增删改O(logN)）来跟踪进程所有待检测的文件描述字**，每次操作时只需要通过 `epoll_ctl()` 传入一个需要监控的 socket，减少了内核和用户空间大量的数据拷贝和内存分配。
2. epoll 使用**事件驱动**的机制，内核里**维护了一个链表来记录就绪事件**，当某个 socket 有事件发生时，通过**回调函数**内核会将其加入到这个就绪事件列表中，当用户调用 `epoll_wait()` 函数时，只将有事件发生的 Socket 集合传递给应用程序，不需要像 select/poll 那样轮询扫描整个 socket 集合，大大提高了检测的效率。



### 阻塞&非阻塞IO

- 阻塞：在进/线程中，发起一个调用时，在调用返回之前，进/线程会被阻塞等待，等待中的进/线让出CPU的使用权。
- 非阻塞：在进/线程中，发起一个调用时，会立即返回。
- 会阻塞的四个函数：connect()、accept()、send()、recv()

阻塞&非阻塞IO的应用场景：

- 在传统的网络服务端程序中（每连接每线/进程），采用阻塞IO
- **在IO复用的模型中，事件循环不能被阻塞在任何环节，所以应该采用非阻塞IO。**

1. **非阻塞IO-connect()**
   - 对非阻塞的IO调用connect()函数，不管是否能连接成功，connect()都会立即返回失败，errno==**EINPROGRESS**
   
   - 对非阻塞的IO调用connect()函数后，如果socket的状态是可写的，证明连接是成功的，否则是失败的。
   
2. **非阻塞IO-accept()**
   - 对非阻塞的IO调用accept()，如果已连接队列中没有socket，函数立即返回失败，errno==**EAGAIN**

3. **非阻塞IO-recv()**
   - 函数对非阻塞的IO调用recv()，如果没数据可读（接收缓冲区为空），立即返回失败，errno==**EAGAIN**

4. **非阻塞IO-send()**
   - 对非阻塞的IO调用send()，如果socket不可写（发送缓冲区已满），立即返回失败，errno==**EAGAIN**



### 水平触发&边缘触发

- select 和 poll 采用水平触发。
- epoll 有水平触发（缺省）和边缘触发两种机制。

**水平触发（level-trigger）：**

- 读事件：如果epoll_wait触发了读事件，表示有数据可读，如果程序没有把数据读完，再次调用epoll_wait的时候，将立即再次触发读事件。
- 写事件：如果发送缓冲区没有满，表示可以写入数据，只要缓冲区没有被写满，再次调用epoll_wait的时候，将立即再次触发写事件。

**边缘触发（edge-trigger）：**

- 读事件：epol wait触发读事件后，不管程序有没有处理读事件epoll_wait都不会再触发读事件，只有当新的数据到达时，才再次触发读事件。
- 写事件：epoll_wait触发写事件之后，如果发送缓冲区仍可以写（发送缓冲区没有满），epoll_wait不会再次触发写事件；只有当发送缓冲区由 满 变成 不满 时，才再次触发写事件。

> 在IO复用的模型中，如果是水平触发模式，某些场景可以用阻塞IO，如果是边缘触发，必须使用非阻塞IO（原因：边缘触发模式下，需要采用while循环来保证处理完所有事件，如果使用阻塞IO将没有办法跳出while，因此使用非阻塞IO，根据其返回值和errno来判断是否跳出while）。但IO复用的本质是不在任何地方阻塞，所以IO复用的模型中，阻塞IO没有意义，并且如果代码写的不好容易出现bug。

一般来说，边缘触发的效率比水平触发的效率要高，因为边缘触发可以减少 epoll_wait 的系统调用次数，系统调用也是有一定的开销的的，毕竟也存在上下文的切换。



## setsockopt()

在服务端程序中，用setsockopt()函数设置socket的属性（一定要放在bind()之前）。

```cpp
// 最简单的epoll服务端程序

// 设置listenfd的属性
int opt = 1;
// 打开地址复用功能：
// 如果当前启动进程绑定的 IP+PORT 与处于TIME_WAIT状态的连接占用的 IP+PORT 存在冲突，但是新启动的进程使用了 SO_REUSEADDR 选项，那么该进程就可以绑定成功
// 绑定的 IP地址 + 端口时，只要 IP 地址不是正好(exactly)相同（例如，0.0.0.0:8888和192.168.1.100:8888），那么允许绑定
setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,static_cast<socklen_t>(sizeof opt));    // 必须的。

// 不使用Nagle算法
// Nagle 算法默认是打开的，如果对于一些需要小数据包交互的场景的程序，比如，telnet 或 ssh 这样的交互性比较强的程序，则需要关闭 Nagle 算法
setsockopt(listenfd,SOL_SOCKET,TCP_NODELAY   ,&opt,static_cast<socklen_t>(sizeof opt));    // 必须的。

// 允许多个进程绑定相同的 IP 地址和端口
setsockopt(listenfd,SOL_SOCKET,SO_REUSEPORT ,&opt,static_cast<socklen_t>(sizeof opt));    // 有用，但是，在Reactor中意义不大。

// 启用TCP保活
setsockopt(listenfd,SOL_SOCKET,SO_KEEPALIVE   ,&opt,static_cast<socklen_t>(sizeof opt));    // 可能有用，但是，建议自己做心跳。
```





## TCP粘包和分包

当用户消息通过 TCP 协议传输时，消息可能会被操作系统分组成多个 TCP 报文（即一个完整的用户消息被拆分成多个 TCP 报文进行传输），send函数只是将数据从应用程序拷贝到了操作系统内核协议栈中，至于什么时候真正被发送，**取决于发送窗口、拥塞窗口以及当前发送缓冲区的大小等条件**。

因此，**我们不能认为一个用户消息对应一个 TCP 报文，正因为这样，所以 TCP 是面向字节流的协议**。

- 粘包：tcp接收到数据之后，有序放在接收缓冲区中，数据之间不存在分隔符的说法，如果接收方没有及时的从缓冲区中取走数据看上去就像粘在了一起。
- 分包：tcp报文的大小缺省是1460 字节，如果发送缓冲区中的数据超过1460字节，tcp将拆分成多个包发送，如果接收方及时的从接收缓冲区中取走了数据，看上去像就接收到了多个报文。

解决方法（应用层来处理）：

1. 采用固定长度的报文
2. 自定义消息结构：例如，在报文前面加上报文长度，报文头部（4字节整数）+报文内容。
3. 报文之间使用分隔符。http协议中用 \r\n\r\n 作为边界



## eventfd 通知线程 

**eventfd 是一个计数相关的fd**。计数不为零是有**可读事件**发生，`read` 之后计数会清零，`write` 则会递增计数器。

**句柄创建**

创建一个eventfd对象，该对象是一个内核维护的无符号的64位整型计数器，初始化为initval的值。

```cpp
#include <sys/eventfd.h>
int eventfd(unsigned int initval, int flags);
```

**flags**可以以下三个标志位的OR结果：

- `EFD_CLOEXEC` : fork子进程时不继承，对于多线程的程序设上这个值不会有错的。
- `EFD_NONBLOCK`: 文件会被设置成O_NONBLOCK，读操作不阻塞。若不设置，一直阻塞直到计数器中的值大于0。
- `EFD_SEMAPHORE` : 支持 semophore 语义的read，每次读操作，计数器的值自减1。

**读写fd**

```cpp
// Write N bytes of BUF to FD.  Return the number written, or -1
ssize_t write (int __fd, const void *__buf, size_t __n);
// Read NBYTES into BUF from FD. Return the number read, -1 for errors or 0 for EOF.
ssize_t read(int __fd, void *__buf, size_t __nbytes);

uint64_t u;
ssize_t n;
// 返回写入/读取的字节数
// 写 eventfd，内部 buffer 必须是 8 字节大小；
n = write(efd, &u, sizeof(uint64_t)); 
// 读 eventfd
n = read(efd, &u, sizeof(uint64_t));
```

读操作

1. 如果计数器中的值大于0：

   - 设置了 `EFD_SEMAPHORE` 标志位，则buf设置为1，且计数器中的值也减去1。

   - 没有设置 `EFD_SEMAPHORE` 标志位，则buf设置为计数器中的值，且计数器置0。

2. 如果计数器中的值为0：

   - 设置了 `EFD_NONBLOCK` 标志位就直接返回-1。errno设置为EAGAIN

   - 没有设置 `EFD_NONBLOCK` 标志位就会一直阻塞直到计数器中的值大于0。

写操作

1. 如果写入值的和不大于0xFFFFFFFFFFFFFFFE，则写入成功

2. 如果写入值的和大于0xFFFFFFFFFFFFFFFE

   - 设置了 `EFD_NONBLOCK` 标志位就直接返回-1。

   - 如果没有设置 `EFD_NONBLOCK` 标志位，则会一直阻塞直到read操作执行

**监听fd**

-  eventfd 计数不为 0 ，那么 fd 是可读的。
- 由于 eventfd 一直可写（可以一直累计计数），所以一直有可写事件

eventfd 如果用 epoll 监听事件，那么都是监听读事件，因为监听写事件无意义。

**关闭fd**

```cpp
#include <unistd.h>
int close(int fd);
```

 



## 正确的使用指针

- 如果资源的生命周期难以确定，则使用shard_ptr来管理。
- 类自己所拥有的资源用unique_ptr来管理，在类被销毁的时候，将会自动释放资源。
- 对于不属于自己、但会使用的资源，采用 unique_ptr& 或 shared_ptr来管理会很麻烦、不易阅读，还可能会对新手带来一系列问题，依旧采用裸指针来管理。



# reactor-netserver项目

## 一、从0实现百万并发的Reactor服务器

1. 最简单的epoll服务端（1）
2. 优化epoll服务端程序，为封装做准备（2）
3. 把网络地址协议封装成**InetAddress**类（3）
4. 把socket的库函数封装成**Socket**类（4）
5. 把epoll的各种操作封装成**Epoll**类（5）
6. 把与TCP连接通道封装成**Channel**类（6、7）

## 二、Reacor模式中的事件驱动机制核心原理

1. 用C++11的function实现函数回调（8）

   在Channel类的HandleEvent函数中处理有数据可读的情况：判断条件（监听fd/连接fd？），然后执行不同的代码。这种传统的处理方法存在一些问题：

   - 如果存在很多种情况，那么就有非常多的分支；
   - 每个分支中的代码逻辑是写死的，无法实现定制的功能。

   改为回调的方法。

2. 把事件循环封装成**EventLoop**类（9）

   一个EventLoop就表示一个server线程，有唯一的epoll句柄（红黑树），可以对应多个Channel，负责事件循环。

3. 把服务端封装成**TcpServer**类（10）

   一个TcpServer可以有多个事件循环，现在是单线程，暂时只用一个事件循环。

   

   为什么需要Acceptor和Connection类？

   Channel(通道)，封装了监听fd和客户端连接的fd。
   监听的fd与客户端连接的fd的功能是不同的，
   监听的fd（永久）与客户端连接的fd（连接断开）的生命周期也不同。

   How to do?

   在Channel之上再做一层封装。

   - 监听Channel封装成**Acceptor**类
     - 水平触发
   - 客户端连接的Channel封装成**Connection**类
     - 边缘触发

4. 把接受客户端连接封装成Acceptor类（11）

5. 把TCP连接封装成Connection类（12）

   

   **回调优化**

6. 在Channel类中回调Acceptor类的成员函数（13）

   在Channel类中创建Connection对象不合适，因为Channel是Acceptor和Conneciton的下层类；

   Acceptor和Conneciton是平级的，在传统的网络服务器中，连接socket由监听socket产生，而Acceptor封装了监听socket，因此Connection对象应该由Acceptor来创建。

7. 在Acceptor类中回调TcpServer类的成员函数（14）

   上一节在Acceptor类中创建了Connection对象conn，但conn不属于Acceptor类，conn的生命周期应该是：客户端连接请求时创建，连接断开时销毁。

   而Acceptor无法感知连接断开，也就无法处理Connection对象的释放。

   实际上，Connection对象属于TcpServer类，因此**在TcpServer类的成员函数中实现Connection对象的创建**，在Acceptor类中回调该函数。

   （此时Connection对象仍没有释放，但后续会解决）

## 三、先实现单线程的Reactor服务器的功能

1. 用map容器管理Connection对象（15）

   一个TcpServer可以有多个Connection对象，在处理客户端连接请求时，把新建的Connection对象存放在map容器中；

   在TcpServer的析构函数中释放全部Connection对象。

2. 在Channel类中回调Connection类的成员函数（16）

   存在问题：不能只能够释放所有Connection对象，**当某个客户端出错或关闭连接时，应该立马释放对应的Connection对象**。

   目前TcpServer无法知道连接关闭或者出错的事件，只有Channel对象知道，但Channel不能删除Connection对象（Connection不属于Channel）

   那如何释放Connection对象呢？采用回调函数！ 回调过程： Channel --> Connection --> TcpServer

3. 在Connection类中回调TcpServer类的成员函数（17）



在非阻塞的网络服务程序中，事件循环不会阻塞在recv和send中，如果数据接收不完整，或者发送缓冲区已填，都不能等待，所以Buffer是必须的。
在Reactor模型中，每个Connection对象拥有InputBuffer和SendBuffer.

muduo作者陈硕：（应该是针对连接Socket为水平触发的情况）

> **TcpConnection 必须要有input buffer**
>
> TCP是一个无边界的字节流协议，接收方必须要处理“收到的数据尚不构成一条完整消息”和“一次收到两条消息的数据”等情况。一个常见的场景是，发送方send()了两条1kB的消息(共2kB)，接收方收到数据的情况可能是:
>
> - 一次性收到 2kB 数据；
> - 分两次收到，第一次 600B，第二次 1400B;
> - 分两次收到，第一次 1400B，第二次600B;
> - 分两次收到，第一次 1kB，第二次 1kB;
> - 分三次收到，第一次 600B，第二次 800B，第三次600B;
> - 其他任何可能。一般而言，长度为n字节的消息分块到达的可能性有2”-1种。
>
> 网络库在处理“socket可读”事件的时候，必须一次性把socket里的数据读完(从操作系统buffer 搬到应用层buffer)，否则会反复触发POLLIN事件，造成busy-loop。**那么网络库必然要应对“数据不完整”的情况，收到的数据先放到 input buffer 里，等构成一条完整的消息再通知程序的业务逻辑**。所以，在TCP网络编程中网络库必须要给每个TCPconnection 配置input buffer。 

> **TcpConnection必须要有output buffer**
>
> 考虑一个常见场景:程序想通过TCP连接发送100kB的数据，但是在write()调用中，操作系统只接受了80kB(受TCP advertised window 的控制，细节见[TCPv1])，你肯定不想在原地等待，因为不知道会等多久(取决于对方什么时候接收数据，然后滑动TCP窗口)。程序应该尽快交出控制权，返回eventloop。在这种情况下，剩余的20kB数据怎么办?
>
> 对于应用程序而言，它只管生成数据，它不应该关心到底数据是一次性发送还是分成几次发送，这些应该由网络库来操心，程序只要调用TcpConnection::send()就行了，网络库会负责到底。**网络库应该接管这剩余的 20kB数据，把它保存在该TCPconnection 的output buffer 里，然后注册 POLLOUT 事件，一旦 socket 变得可写就立刻发送数据**。当然，这第二次 write()也不一定能完全写入 20kB，如果还有剩余，网络库应该继续关注POLLOUT事件；如果写完了20kB，网络库应该停止关注POLLOUT以免造成 busy loop。

1. 网络编程为什么需要缓冲区Buffer : TCP粘包和分包（18）

2. 封装缓冲区Buffer类（18）

   在Connection类中增加InputBuffer和SendBuffer成员变量；

   **将读取数据的函数OnMessage从Channel类中转移到Connection类，这样Connection类才能使用buffer。**

   （另外，修复了一个bug：客户端ip和port的获取）

3. 使用接收缓冲区inputbuffer（19）

   Connection是底层类，不应该承担数据计算（消息处理）的工作，暂时将该部分交由TcpServer类（也是底层类，其中的最上层，是网络库的一部分）实现，使用回调函数。

4. 使用发送缓冲区outputbuffer（20）

   在Connection类中实现处理写事件的函数，在Channel类中回调该函数。

5. 优化回调函数（21）

   目前为止：

   ![image-20240316232855688](https://gitee.com/dengquan02/images/raw/master/image-20240316232855688.png)

   Channel中的这四次回调过程对应了TCP连接的四种事件，但在网络编程中，有些不是TCP连接的事件也应该关心。两个例子：

   - 发送数据：在TcpServer中调用Connection::Send就可以了，网络库会负责到底，但TcpServer可能需要知道数据是否发送成功。
   - epoll_wait 返回失败或超时：也应该通知TcpServer。

   采用回调函数的方法。

   解决了发送数据和epoll_wait超时的情况（epoll_wait返回失败未考虑）。

6. 实现回显服务器EchoServer（22）

   为了支持不同的业务，可以在TcpServer类之上创建业务类，实现不同的业务逻辑。

   EchoServer是目前唯一的业务类，实现了回显业务。


## 四、线程池技术实现多线程的Reactor服务器

1. 简单优化Buffer（23）

2. 封装线程池**ThreadPool**类（24）

3. 多线程的主从Reactor模型（25）

   一个TcpServer可以有多个事件循环，在此之前是单线程，暂时只用一个事件循环。单线程的Reactor模型不能发挥多核CPU的性能，现在改为多线程。

   - 运行多个事件循环，**主事件循环运行在主线程中，从事件循环运行在线程池中**。
   - **主线程负责创建客户端连接，然后把conn分配给线程池**。

   在TcpServer中增加从Reactor的线程池。

4. 增加工作线程（26）

   Why？

   Acceptor运行在主Reactor(主进程)中，Connection运行在从Reactor(线程池)中。
   一个从Reactor负责多个Connection，每个Connection的工作内容包括IO和计算(处理业务)。IO不会阻塞事件循环，但是，计算可能会阻塞事件循环。如果计算阻塞了事件循环，那么，在同一Reactor中的全部Connection将会被阻塞。

     ![image-20240322185140834](https://gitee.com/dengquan02/images/raw/master/image-20240322185140834.png)

   从Reactor负责IO，工作线程负责计算。

   这种模式，使得从Reactor就不会阻塞在计算上，但也不能解决所有问题：

   例如，如果同时有多个业务需要处理，且都需要较长时间，那么所有的工作线程都将被阻塞。

   但这并不是网络库的错，如果要实现高并发，处理单个业务的时间不应太长，应该优化业务。对于网络库而言，这已经是最好的设计方案了。

   在EchoServer中增加**工作线程池**。

## 五、优化Reactor服务器的种种细节

1. 在多线程中如何管理资源（27）

   conn在主线程（主Reactor）中创建，在IO线程（从Reactor）中使用，收到请求报文（有业务需要处理）时，conn作为函数参数传递给工作线程。如果此时Tcp连接已经断开，则conn在IO线程中被释放，工作线程就将使用野指针，结果不可预知。

   因此，conn不应该在IO线程中释放，使用**智能指针**来解决。

2. 用**shared_ptr**管理共享资源（28）

   对Connection对象使用shared_ptr管理，这样就解决了野指针的问题。

   但这产生了一个新的问题：Tcp连接断开和Connection对象析构之间有时间差，在该时间差内关注Tcp连接的任何事件没有意义，还可能引起程序bug，因此**在连接断开后**应该取消关注该连接的所有事件（既然都要删除了，这一步可以不用），并且**把它从事件循环中删除**（即删除Connection对象对应的Channel在所属epoll句柄-红黑树中的注册）。

3. 用**unique_ptr**管理自己的资源（29）

   用unique_ptr管理其他对象。

   其中，在Acceptor、Connection、Channel类中都用到了EventLoop对象，但他们对loop的资源没有所有权，因此使用裸指针是可行的。当然也可以使用智能指针，但只能使用智能指针的（常）引用。

4. 用**eventfd**实现事件通知

   - 通知线程的方法：条件变量、信号量、Socket、管道、eventfd
   - 事件循环阻塞在epoll_wait()函数，条件变量、信号量有自己的等待函数，不适合用于通知事件循环。（在同一个线程中，不可能同时调用不同的等待函数）
   - Socket、管道、eventfd都是实现了 `file_operation->poll` 的调用的“文件”fd，可以被epoll（专门用于管理事件的池子）管理，如果要通知事件循环，往Socket、管道、eventfd中写入数据即可。

5. **异步唤醒事件循环**（30）

   **服务端发送信息时**：

   - 调用Connection::Send，将需要发送的数据保存在output_buffer_，注册写事件
   - （一旦Socket可写）调用处理写事件的Connection::Write：发送output_buffer_中的数据，并从中删除成功发送的部分

   注意到，Send运行在工作线程，Write运行在IO线程，且两者都操作output_buffer_，这会产生竞争。

   对output_buffer_加锁不是好方法，因为对网络服务端程序来说，可能会有上百万个Connection，每个Connection都有自己的发送缓冲区，这样就需要大量的锁，锁的开销是很大的，太浪费资源了。

   有更好的方法：**工作线程处理完业务后，把发送数据的操作交给IO线程**

   ![image-20240327171400757](https://gitee.com/dengquan02/images/raw/master/image-20240327171400757.png)

   1. 给每个IO线程增加一个任务队列
   2. 工作线程处理完业务后，把发送数据的操作添加到对应IO线程的任务队列中，并使用eventfd唤醒该IO线程（此时IO线程阻塞在epoll_wait中），让IO线程（EventLoop）去执行发送任务。

   

   **连接被动断开时**，

   - 在Connection::onMessage中感知到，调用Connection::closeConnection，接着回调TcpServer::closeConnection，此时在SubEventLoop线程中运行。
   - closeConnection所做工作是从TcpServer对象中删除当前Connection对象，而TcpServer对象是属于MainEventLoop的，因此应当在MainEventLoop中运行closeConnectionInLoop（使用上面一样的线程切换技术）。这也是贯彻“**One Loop Per Thread**”的理念。

6. 性能优化-阻止浪费，**清除空闲的TCP连接**（31）

   业务需求：

   空闲的connection对象是指长时间没有进行通讯的tcp连接，会占用资源，需要（在主事件循环中）定期清理，这样还可以避免攻击。

   定时器&信号：

   - 定时器用于执行定时任务，例如清理空闲的tcp连接
   - 传统的做法：alarm()函数可设置定时器，触发SIGALRM信号。
   - 新版Linux内核把定时器和信号抽象为fd，让epoll统一监视

   

   1. 增加**Timestamp**类，为Connection类增加Timestamp对象，每收到一次报文就更新时间戳
   2. 在EventLoop类中设置定时器，闹钟时间间隔和超时时间参数化
   3. 在EventLoop类中增加`map<int, spConnect> conns_`，存放运行在该事件循环上的全部Connection对象
   4. 如果闹钟时间到了，遍历`conns_`，判断每个Connection对象是否超时
   5. 如果超时，则从EventLoop和TcpServer的map容器`conns_`中删除该对象
   6. EventLoop和TcpServer的map容器需要加锁。因为：
      - 创建新连接时在主线程中操作 `conns_`，释放连接时在子线程中操作`conns_`

## 六、真金不怕火炼-在正式PC服务器测试，见证每秒百万并发

1. 让多线程的网络服务程序体面的退出（32）

   退出之前应该做一些收尾工作：保存数据，释放资源......

   设置2和15信号的处理函数，在函数中停止主从事件循环和工作线程，然后服务程序主动退出。

2. 设计更高效的Buffer（33）

   阅读陈硕muduo 7.4

3. 性能测试（百万QPS）（34）

4. 基于Reactor服务器开发业务（网上银行服务器示例）（35）

   





















































