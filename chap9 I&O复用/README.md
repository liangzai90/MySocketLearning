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


## 9.1 select系统调用

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


































