# 第9章 I/O复用

I/O复用使得程序能同时监听多个文件描述符，这对提高程序的性能至关重要。通常，网络程序在下列情况下需要使用I/O复用技术：

- [ ] 客户端程序要同时处理多个socket。比如本章将要讨论的非阻塞connect技术。

- [ ] 客户端程序要同事处理用户输入和网络连接。比如本章将要讨论的聊天室程序。

- [ ] TCP服务器要同时处理监听socket和连接socket。这是I/O复用使用最多的场合。后续章节将展示很多这方面的例子。

- [ ] 服务器要同事处理TCP请求和UDP请求。比如本章将要讨论的回射服务器。

- [ ] 服务器要同事监听多个端口，或者处理多种服务。比如本章将要讨论的xinetd服务器。

需要指出的是，I/O复用虽然能同时监听多个文件描述符，但它本身是阻塞的。并且当多个文件描述符同时就绪时，
如果不采取额外的措施，程序就只能按顺序依次处理其中的每一个文件描述符，
这使得服务器程序看起来像是串行工作的。
如果要实现并发，只能使用多进程或多线程等编程手段。

Linux下实现I/O复用的系统调用主要有**select**、**poll**和**epoll**，本章将依次讨论，然后介绍使用它们的几个实例。


----------------------------------------------------------------


## 9.1 select 系统调用

select 系统调用的用途是：在一段指定时间内，监听用户感兴趣的文件描述符上的可读、可写和异常等事件。

#### 9.1.1 select API

select系统调用的原型如下：

```C++
#include <sys/select.h>

int  select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);
```

1) nfds参数指定被监听的文件描述符的总数。它通常被设置为select监听的所有文件描述符中的最大值加1，
因为文件描述符是从0开始计数的。

2) readfds、writefds和exceptfds参数分别指向可读、可写和异常等事件对应的文件描述符集合。
应用程序调用select函数时，通过这3个参数传入自己感兴趣的文件描述符。
select调用返回时，内核将会修改它们来通知应用程序哪些文件描述符已经就绪。
这3个参数是fd_set结构指针类型。

fd_set结构体的定义如下：

```C++
#include  <typesizes.h>
#define  __FD_SETSIZE  1024

#include  <sys/select.h>
#define  FD_SETSIZE  __FD_SETSIZE

typedef  long  int  __fd_mask;
#undef  __NFDBITS
#define  __NFDBITS  (8 * (int)sizeof(__fd_mask))

typedef struct
{
#ifdef  __USE_XOPEN
    __fd_mask  fds_bits[ __FD_SETSIZE / __NFDBITS ];
#define  __FD_BITS(set)  ((set)->fds_bits)    
#else
    __fd_mask  __fds_bits[ __FD_SETSIZE / __NFDBITS ];
#define  __FDS_BITS(set)  ((set)->__fds_bits)
#endif
} fd_set;
```

由以上定义可见，fd_set结构体仅包含一个整形数组，该数组的每个元素的每一位（bit）标记一个文件描述符。
fd_set 能容纳的文件描述符数量由 FD_SETSIZE 指定，这就限制了select能同时处理的文件描述符的总量。

由于位操作过于烦琐，我们应该使用下面的一系列宏来访问 fd_set 结构体中的位：

```C++
#include  <sys/select.h>

FD_ZERO( fd_set  *fdset);  //清除 fdset 的所有位
FD_SET( int fd, fd_set *fdset );//设置 fd_set 的位 fd
FD_CLR( int fd, fd_set *fdset );//清除 fd_set 的位 fd
int  FD_ISSET( int fd, fd_set *fdset );//测试 fd_set 的位 fd是否被设置
```
3) timeout参数用来设置select函数的超时时间。
timeval结构体定义如下：

```C++
struct timeval
{
    long  tv_sec; //秒数
    long  tv_usec; //微妙数
};
```

如果给timeout变量的tv_sec和tv_usec成员都传递0，则select将立即返回。
如果给timeout传递NULL，则select将一直阻塞，知道某个文件描述符就绪。

* select成功时返回就绪（可读、可写和异常）文件描述符的总数。如果在超时时间内没有任何文件描述符就绪，
select将返回0.
* select失败时返回-1，并设置errno。

如果在select等待期间，程序接收到信号，则select立即返回-1，并设置errno为EINTR。


### 9.1.2 文件描述符就绪条件

哪些情况下文件描述符可以被认为是可读、可写或者出现异常。
对于select的使用非常关键。


在网络编程中，下列情况下 **socket可读**：

- [ ] socket 内核接收缓冲区中的字节数大于或等于其低水位标记SO_RCVLOWAT。此时我们可以无阻塞地读该socket，
并且读操作返回的字节数大于0。

- [ ] socket通信的对方关闭连接。此时对该socket的读操作将返回0。

- [ ] 监听socket上有新的连接请求。

- [ ] socket上有未处理的错误。此时我们可以使用getsockopt来读取和清除该错误。


下列情况下 **socket 可写**：

- [ ] socket内核发送缓冲区中的可用字节数大于或等于其低水位标记SO_SNDLOWAT。
此时我们可以无阻塞地写该socket，并且写操作返回的字节数大于0。

- [ ] socket 的写操作被关闭。对写操作被关闭的socket执行写操作将触发一个SIGPIPE信号。

- [ ] socket使用非阻塞connect连接成功或失败（超时）之后。

- [ ] socket上有未处理的错误。此时我们可以使用getsockopt来读取和清除该数据。

网络编程中，select能处理的异常情况只有一种：socket上接收到带外数据。


## 9.2 poll 系统调用

poll系统调用和select类似，也是在指定时间内轮询一定数量的文件描述符，以测试其是否有就绪。

poll的原型如下：

```C++
#include  <poll.h>

int  poll( struct pollfd* fds, nfds_t nfds, int timeout );
```

1) fds参数是一个pollfd结构体类型的数组，它指定所有我们感兴趣的文件描述符上发生的可读、可写和异常等事件。

pollfd结构体的定义如下：

```C++
struct  pollfd
{
    int fd;  //文件描述符
    short events;  // 注册的事件
    short revents;  // 实际发生的事件，由内核填充
};
```

其中，fd成员指定文件描述符；
events成员告诉poll监听fd上的哪些事件，它是一系列事件的按位或；
revents成员则由内核修改，以通知应用程序fd上实际发生了哪些事情。

2) nfds 参数指定被监听事件集合fds的大小，其类型nfds_t的定义如下：

```C++
typedef  unsigned  long  int  nfds_t;
```
3) timeout 参数指定poll的超时值，单位是毫秒。
当timeout为-1时，poll调用将永远阻塞，直到某个事件发生；
当timeout为0时，poll调用将立即返回。

poll系统调用的返回值的含义与select相同。


----------------------------------------------------------------------------

## 9.3 epoll 系列系统调用

### 9.3.1 内核事件表

epoll是Linux特有的I/O复用函数。
它在实现和使用上与select、poll有很大差异。
首先，epoll使用一组函数来完成任务，而不是单个函数。
其次，epoll把用户关心的文件描述符上的事件放在内核里的一个事件表中，
从而无须像select和poll那样每次调用都要重复传入文件描述符集火事件集。
但epoll需要使用一个额外的文件描述符，来唯一标识内核中的这个事件。

这个文件描述符使用如下epoll_create函数来创建：

```C++
#include  <sys/epoll.h>

int  epoll_create( int size );
```
size参数现在并不起作用，只是给内核一个提示，告诉他事件表需要多大。
**该函数返回的文件描述符将用作其他所有epoll系统调用的第一个参数，以指定要访问的内核事件表**。

下面的函数用来操作 epoll 的内核事件表:

```C++
#include  <sys/epoll.h>

int  epoll_ctl( int epfd, int op, int fd, struct epoll_event *event );
```

fd参数是要操作的文件描述符，op参数则指定操作类型。操作类型有如下3种：

- [ ] EPOLL_CTL_ADD，往事件表中注册 fd 上的事件。

- [ ] EPOLL_CTL_MOD，修改 fd 上的注册事件。

- [ ] EPOLL_CTL_DEL，删除 fd 上的注册事件。

event 参数指定事件，它是epoll_event结构体指针类型。
epoll_event的定义如下：

```C++
struct  epoll_event
{
    __uint32_t  events;  //epoll事件
    epoll_data_t  data; //用户数据
};
```


其中events成员描述事件类型。
data成员用于存储用户数据，
其类型为 epoll_data_t 的定义如下：

```C++
typedef  union  epoll_data
{
    void*  ptr;
    int  fd;
    uint32_t  u32;
    uint64_t  u64;
}epoll_data_t;
```










































