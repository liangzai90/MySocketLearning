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

### 9.1.1 select API

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

epoll_data_t是一个联合体，其4个成员中使用最多的是fd，它指定事件所从属的目标文件描述符。
ptr成员可用来指定与fd相关的用户数据。

* epoll_ctl成功时返回0，是吧则返回-1，并设置errno。



### 9.3.2 epoll_wait 函数

epoll系列系统调用的主要接口是 epoll_wait 函数。它在一段超时时间内等待一组文件描述符上的事件，
其原型如下：

```C++
#include  <sys/epoll.h>

int  epoll_wait( int epfd, struct epoll_event* events, int maxevents, int timeout );
```

函数成功时返回就绪的文件描述符的个数，失败时返回-1并设置errno。

maxevents参数指定最多监听多少个事件，它必须大于0。

epoll_wait函数如果检测到事件，就将所有就绪的事件从内核事件表（由epfd参数指定）中复制到它的第二个参数events指向的数组中。
这个数组**只用于输出epoll_wait检测到的就绪事件**）。而不像select和poll的数组参数那样，
即用于传入用户注册的事件，又用于输出内核检测到的就绪事件。
这就极大地提高了应用程序索引就绪文件描述符的效率。

```C++
// poll 和 epoll 在使用上的差别

//如何索引 poll 返回的就绪文件描述符
int ret = poll( fds, MAX_EVENT_NUMBER, -1 );
// 必须遍历所有已注册文件描述符并找到其中的就绪者（当然，可以利用ret来稍作优化）
for(int i=0; i<MAX_EVENT_NUMBER; ++i)
{
    if(fds[i].revents & POLLIN)  //判断第i个文件描述符是否就绪
    {
        int sockfd = fds[i].fd;
        ///处理sockfd
    }
}


//如何索引 epoll 返回的就绪文件描述符
int ret = epoll_wait( epollfd, events, MAX_EVENT_NUMBER, -1 );
// 仅遍历就绪的 ret 个文件描述符
for(int i=0; i<ret; i++)
{
    int sockfd = events[i].data_fd;
    ///sockfd肯定就绪，直接处理
}

```

### 9.3.3 LT和ET模式

epoll对文件描述符的操作有两种模式：LT(Level Trigger，电平触发)模式和ET(Edge Trigger，边沿触发)模式。
LT模式是默认的工作模式，这种模式下epoll相当于一个效率较高的poll。
当往epoll内核事件表中注册一个文件描述符上的EPOLLET事件时，epoll将以ET模式来操作该文件描述符。
ET模式是epoll的高效工作模式

对于采用LT工作模式的文件描述符，当epoll_wait检测到其上右事件发生并将此事件通知应用程序后，
应用程序可以不立即处理该事件。这样，应用程序下一次调用epoll_wait时，epoll_wait还会再次向应用程序
通告此事件，直到该事件被处理。

而对于采用ET工作模式的文件描述符，当epoll_wait检测到其上有事件发生时并将此事件通知应用程序后，
**应用程序必须立即处理该事件**，因为后续的epoll_wait调用将不再向应用程序通知这一事件。

ET模式在很大程度上降低了同一个epoll事件被重复触发的次数，因此效率要比LT模式高


### 9.3.4 EPOLLONESHOT 事件

即使使用了ET模式，一个socket上的某个事件还是可能被触发多次。

我们期望的是一个socket连接在任意一时刻都只被一个线程处理。这一点可以使用epoll的 EPOLLONESHOT 事件实现。

对于注册了EPOLLONESHOT事件的文件描述符，操作系统最多触发其上注册的一个可读、可写或者异常事件，且只触发一次，
除非我们使用 epoll_ctl 函数重置该文件描述符上注册的EPOLLONESHOT事件。这样，当一个线程在处理某个socket时，
其他线程时不可能有机会操作该socket的。



----------------------------------------------------------------------------

## 9.4 三组I/O复用函数的比较

* select的参数类型fd_set没有将文件描述符和事件绑定，它仅仅是一个文件描述符集合，
因此select需要提供3个这种类型的参数来分别传入和输出可读、可写及异常等事件。

* poll的参数类型pollfd则多少“聪明”一些。它把文件描述符和事件都定义其中，任何事件都被统一处理，
从而使得编辑接口简洁得多。

* epoll则采用与select和poll完全不同的方式来管理用户注册事件。它在内核中维护一个事件表，
并提供了独立的系统调用epoll_ctl来控制往其中添加、删除、修改事件。

从实现原理上来说，select和poll采用的都是轮询的方式，即每次调用都要扫描整个注册文件描述符集合，
并将其中就绪的文件描述符返回给用户程序，因此它们检测就绪事件的时间复杂度是O(n)

epoll_wait则不同，它采用的是回调的方式。
内核检测到就绪的文件描述符时，将触发回调函数，回调函数就将该文件描述符上对应的事件插入内核就绪事件队列。
内核最后在适当的实际将该就绪事件队列中的内容拷贝到用户控件。因此epoll_wait无需轮询整个文件描述符集合来
检测哪些事件已经就绪，其算法事件复杂度是O(1)。

epoll_wait适用于连接数量多， 但活动连接少的情况。


## 9.5 I/O复用的高级应用之一：非阻塞connect

## 9.6 I/O复用的高级应用二：聊天室程序

以poll为例实现一个简单的聊天室程序，以阐述如何使用I/O复用技术来同时处理
**网络连接**和**用户输入**。

### 9.6.1 客户端

客户端程序使用poll同时监听用户输入和网络连接，并利用splice函数将用户输入内容直接定向到网络连接上以发送，
从而实现数据零拷贝，提高了程序执行效率。

### 9.6.2 服务器

服务器程序使用poll同时管理监听socket和连接socket，并且使用牺牲空间换取时间的策略来提高服务器性能。


## 9.7 I/O复用的高级应用三：同时处理TCP和UDP服务

超级服务inetd和android的调试服务adbd，可以监听多个端口。

从bind系统调用的参数来看，一个socket只能与一个socket地址绑定，即一个socket只能用来监听一个端口。
因此，服务器如果要同时监听多个端口，就必须创建多个socket，并将它们分别绑定到各个端口上。
这样一来，服务器程序就需要同时管理多个监听socket，I/O复用技术就有了用武之地。


## 9.8 超级服务 xinetd 

Linux因特网服务inetd是超级服务。它同时管理着多个子服务，即监听多个端口。

### 9.9.1 xinetd 配置文件

xinetd采用 /etc/xinetd.conf 主配置文件和 /etc/xinetd.d目录下的子配置文件来管理所有服务。

每一个子配置文件用于设置一个子服务的参数。比如，telent子服务的配置文件 /etc/xinetd.d/telent的典型内容如下：

```bash
# default: on
# description: The telnet server serves telnet sessions; it uses \
#	unencrypted username/password pairs for authentication.
service telnet
{
	disable	= no
	flags		= REUSE
	socket_type	= stream        
	wait		= no
	user		= root
	server		= /usr/sbin/in.telnetd
	log_on_failure	+= USERID
}

```


### 9.8.2 xinetd 工作流程

xinetd管理的子服务中有的是标准服务，比如时间日期服务daytime、回射服务echo和丢弃服务discard。
xinetd服务器在内部直接处理这些服务。
还有的子服务则需要调用外部的服务器程序来处理。
xinetd通过调用fork和exec函数来加载运行这些服务器程序。
比如telent 、ftp服务都是这种类型的子服务。

