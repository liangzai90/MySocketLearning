# 第5章 Linux网络编程基础API

探讨Linux网络编程基础API与内核中TCP/IP协议族之间的关系，并未后续章节提供编程基础。从3个方面讨论Linux网络API.

- [ ] socket地址API。socket 最开始的含义是一个IP地址和端口对（ip, port）。它唯一地表示了使用TCP通信的一端。本书称其为socket地址。

- [ ] socket基础API。socket的主要API都定义在 sys/socket.h 头文件中，包括创建socket、命名socket、监听socket、接受连接、发起连接、读写数据、
获取地址信息、检测带外标记，以及读取和设置socket选项。

- [ ] 网络信息API。Linux 提供了一套网络信息API，以实现主机名和IP地址之间的转换，以及服务名称和端口号之间的转换。
这些API都定义在 netdb.h 头文件中，我们将讨论其中几个主要的函数。

-----------------------------------------------------------------


### 1.主机字节序和网络字节序

大端字节序，也称网络字节序。网络上传输的数据，都是网络字节序的。
```C++
#include <stdio.h>

void byteorder()
{
	union MyUnion
	{
		short value;
		char union_bytes[sizeof(short)];
	} test;

	test.value = 0x0102;
	if ((test.union_bytes[0] == 1) && (test.union_bytes[1] == 2))
	{
		printf("Big  endian. \r\n");// **网络字节序** ，大端对齐（高位在前面）
	}
	else if ((test.union_bytes[1] == 1) && (test.union_bytes[0] == 2))
	{
		printf("little  endian. \r\n");//主机字节序，小端对齐
	}
	else
	{
		printf("unkonwn...\r\n");
	}
}
```


Linux 提供了如下4个函数来完成主机字节序和网络字节序之间的转换
```C++
#include <netinet/in.h>

unsigned long int htonl(unsigned long int hostlong);
unsigned short int htons(unsigned short int hostshort);

unsigned long int ntohl(unsigned long int netlong);
unsigned short int ntohs(unsigned short int netshort);
```

htonl means "host to network long".主机字节序数据转为网络字节序数据。


-----------------------------------------------------------------


### 2.通用socket地址

socket网络编程接口中表示socket地址的结构体 **sockaddr**，其定义如下：
```C++
#include <bits/socket.h>

struct sockaddr
{
    sa_family_t  sa_family;
    char  sa_data[14];
};
```

Linux定义了新的通用socket地址结构体，而且还是内存对齐的（__ss_aligin成员的作用）
```C++
#include <bits/socket.h>

struct sockaddr_storage
{
    sa_family_t  sa_family;
    unsigned long int __ss_aligin;
    char  __ss_padding[128-sizeof(__ss_aligin)];
};
```


-----------------------------------------------------------------


### 3.专用socket地址

Linux为各个协议族提供了专门的socket地址和结构
```C++
#include <sys/un.h>

struct sockaddr_un
{
    sa_family_t  sin_family; //地址族:AF_UNIX
    char sun_path[108];      //文件路径名
};
```

IPv4的专用socket地址结构体 **sockaddr_in** 
```C++
struct sockaddr_in
{
    sa_family_t  sin_family;   //地址族：AF_INET
    u_int16_t  sin_port;       //端口号，要用网络字节序表示
    struct  in_addr  sin_addr;  //IPv4地址结构体
};

struct in_addr
{
    u_int32_t  s_addr;    //IPv4地址，要用网络字节序表示
};
```

IPv6的专用socket地址结构体  **sockaddr_in6**
```C++
struct sockaddr_in6
{
    sa_family_t  sin6_family;   //地址族：AF_INET6
    u_int16_t  sin6_port;       //端口号，要用网络字节序表示
    u_int32_t  sin6_flowinfo;   //流信息，应设置为0
    struct  in6_addr  sin6_addr;  //IPv6地址结构体
    u_int32_t  sin6_scope_id;     //scope ID，尚处于试验阶段
};

struct in6_addr
{
    unsigned  char  sa_addr[16];    //IPv6地址，要用网络字节序表示
};
```

所有专用socket地址（以及sockaddr  storage）类型的变量
在实际使用时都要转化为通用socket地址类型 **sockaddr** （强制转换即可），
因为 **所有socket编程接口使用的地址参数的类型都是sockaddr**。



-----------------------------------------------------------------


### 4.IP地址转换函数


**仅适用于IPv4地址**
```C++
#include <arpa/inet.h>

in_addr_t  inet_addr(const char * strptr);
int  inet_aton(const char* cp, struct in_addr* inp);
char*  inet_ntoa(struct in_addr  in);
```
**inet_addr**函数将用点分十进制字符串表示的IPv4地址转化为用网络字节序整数表示的IPv4地址。
* 失败时返回INADDR_NONE。

**inet_aton**函数完成和inet_addr同样的功能，但是将转化结果存储于参数inp指向的地址结构中。
* 成功时返回1，失败则返回0。

**inet_ntoa**函数将用网络字节序整数表示的IPv4地址转化为用点分十进制字符串表示的IPv4地址。

**inet_ntoa不可重入，非线程安全**，该函数内部用一个静态变量存储转化结果， 函数返回值指向该静态内存。


**同时适用于IPv4和IPv6地址**
```C++
#include <arpa/inet.h>

int  inet_pton(int af, const char* src, void* dst);
const char* inet_ntop(int af, const void* src, char* dst, socklen_t cnt);
```
**inet_pton**函数将用字符串表示的IP地址src（用点分十进制字符串表示的IPv4地址
或用十六进制字符串表示的IPv6地址）转换成用网络字节序整数表示的IP地址，并把转换结果存储于dst
指向的内存中。其中，

af参数指定地址族，可以使AF_INET或者AF_INET6.

* inet_pton成功时返回1，失败则返回0并设置errno。

**inet_ntop**函数进行相反的转换，前3个参数的含义与inet_pton的参数相同，最后一个参数cnt
指定目标存储单元大小。下面的2个宏可以帮助我们指定这个大小（分别用于IPv4和IPv6）

* inet_ntop成功时返回目标存储单元的地址，失败则返回NULL并设置errno。


```C++
#include <netinet/in.h>

#define  INET_ADDRSTRLEN   16
#define  INET6_ADDRSTRLEN  46
```


**举个IP地址转换的例子**
```C++
    address.sin_port = htons(port);//little to big
    inet_pton(AF_INET, ip, &address.sin_addr);
    
    char dest[100] ;
    inet_ntop(AF_INET, &peerHost.sin_addr,dest,100);

```

-----------------------------------------------------------------

### 5.创建socket

UNIX/Linux的一个哲学是：所有的东西都是文件。socket也不例外，它就是可读、可写、
可控制、可关闭的文件描述符。下面的socket系统调用可创建一个socket:
```C++
#include <sys/types.h>
#include <sys/socket.h>

int socket(int domain, int type, int protocol);

eg:
int sock = socket(PF_INET, SOCK_STREAM, 0);

```

domain 参数告诉系统使用哪个底层协议族

type 参数指定服务器类型（SOCK_STREAM, SOCK_DGRAM）

protocol 参数是在前面两个参数构成的协议集合下，再选择一个具体协议。通常为0，使用默认协议。

* socket系统调用成功时返回一个socket文件描述符，失败则返回-1，并设置errno。


-----------------------------------------------------------------


### 6.命名socket

创建socket时，我们给它指定了地址族，但是并未指定使用该地址族中的哪个具体socket地址。
**将一个socket与socket地址绑定成为给socket命名**。

在服务器程序中，我们通常要命名socket，因为只有命名之后客户端才能知道该如何连接它。
客户端则通常不需要命名socket，而是采用**匿名方式**，即使用操作系统自动分配的socket地址。

命名socket的系统调用时 **bind**，其定义如下：

```C++
#include <sys/types.h>
#include <sys/socket.h>

int bind(int sockfd, const struct sockaddr* my_addr, socklen_t addrlen);
```

bind将my_addr所指的socket地址分配给未命名的sockfd文件描述符，addrlen参数支出该socket地址的长度。
* bind 成功时返回0，失败则返回-1并设置errno。


-----------------------------------------------------------------

### 7.监听socket

socket被命名之后，还不能马上接受客户端连接，我们需要使用如下系统调用来创建一个监听队列以存放待处理的客户端连接：

```C++
#include <sys/socket.h>

int listen(int sockfd, int backlog);
```

socket参数指定被监听的socket。

backlog参数提示内核监听队列的最大长度。

* listen成功时返回0，失败则返回-1,并设置errno。

半连接状态：SYN_RCVD
完全连接状态：ESTABLISHED


-----------------------------------------------------------------

### 8.接受连接

下面的系统调用从listen监听队列中接受一个连接：

```C++
#include <sys/types.h>
#include <sys/socket.h>

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```
sockfd参数是执行过listen系统调用的监听socket。

addr参数用来获取被接受连接的远端socket地址，该socket地址的长度由addrlen参数指出。

accept成功时**返回一个新的连接socket**，该socket唯一地标识了被接受的这个连接，
**服务器可以通过读写该socket来与被接受连接对应的客户端通信**。

* accept失败时返回-1，并设置errno。


-----------------------------------------------------------------

### 9.发起连接

如果说服务器通过listen调用来被动接受连接，那么客户端需要通过如下系统调用来主动与服务器建立连接：

```C++
#include <sys/types.h>
#include <sys/socket.h>

int connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);
```
sockfd参数由socket系统调用返回一个socket

serv_addr参数是服务器监听的socket地址

addlen参数则指定这个地址的长度

**一旦成功建立连接，sockfd 就唯一表示了这个连接，客户端就可以通过读写sockfd来与服务器通信**。

* connect成功时返回0。失败则返回-1，并设置errno。


-----------------------------------------------------------------

### 10.关闭连接

关闭一个连接实际上就是关闭该连接对应的socket，这可以通过如下关闭普通文件描述符的系统调用来完成：

```C++
#include <unistd.h>

int close(int fd);
```
fd参数是待关闭的socket。

不过，close系统调用并非总是立即关闭一个连接，而是将fd的引用计数减1.只有当fd的引用计数为0时，才真正关闭连接。
在多进程中，一次fork系统调用默认将是父进程中打开的socket的引用计数加1，因此**必须在父进程和子进程中
都对该socket执行close调用才能真正将连接关闭**。


无论如何都要立即终止连接（而不是将socket的引用计数减1），可以使用如下的shutdown系统调用：
```C++
#include <sys/socket.h>

int shutdown(int sockfd, int howto);
```
sockfd参数是待关闭的socket。

howto参数决定了shutdown的行为。

* shutdown成功时返回0，失败则返回-1，并设置errno。


-----------------------------------------------------------------

### 11.TCP数据读写

对文件的读写操作read和write同样适用于socket。但是socket编程接口提供了几个专门用于socket数据读写的系统调用，
它们增加了对数据读写的控制。
其中适用于TCP流数据读写的系统调用是：

```C++
#include <sys/types.h>
#include <sys/socket.h>

ssize_t  recv(int sockfd, void *buf, size_t len, int flags);
ssize_t  send(int sockfd, const void *buf, size_t len, int flags);
```
recv读取sockfd上的数据，
buf和len参数分别制定读写缓冲区的位置和大小，
flags参数通常设置为0。
recv可能返回0，这意味着通信对方已经关闭连接了。
recv读取到的数据可能小于期望的长度，因此可能需要多次调用recv，才能读取到完整的数据。

* recv 成功时返回实际读取到的数据的长度，出错时返回-1，并设置errno。

send往sockfd上写入数据，
buf和len参数分别指定写缓冲区的位置和大小。
flags参数为数据收发提供了额外的控制（MSG_MORE）
* send成功时返回写入的数据长度，失败则返回-1，并设置errno。


-----------------------------------------------------------------

### 12.UDP数据读写

socket编程接口中用于UDP数据报读写的系统调用是：

```C++
#include <sys/types.h>
#include <sys/socket.h>

ssize_t  recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
ssize_t  sendto(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* dest_addr, socklen_t* addrlen);
```
recvfrom读取sockfd上的数据，buf和len参数分别指定读缓冲区的位置和大小。

UDP通信没有连接的概念，每次读取数据都需要获取发送端的socket地址，即参数src_addr所指的内容，addrlen参数则指定该地址的长度。



------------------------------------------------------------------


### 13.通用数据读写函数

socket编程接口还提供了一对通用的数据读写系统调用。它们不仅能用于TCP流数据，也能用于UDP数据报：

```C++
#include <sys/socket.h>

ssize_t  recvmsg(int sockfd, struct msghdr* msg, int flags);
ssize_t  sendmsg(int sockfd, struct msghdr* msg, int flags);
```
sockfd参数指定被操作的目标socket。
msg参数是msghdr结构体类型的指针
falgs与前面recv,send的相同。

msghdr结构体定义如下：

```C++
struct msghdr
{
    void*  msg_name;  //socket地址
    socklen_t  msg_namelen; //socket地址的长度 
    struct  iovec*  msg_iov;  //****分散的内存块 *****
    int  msg_iovlen;       //分散内存块的数量
    void*  msg_control;    //指向辅助数据的起始位置
    socklen_t  msg_controllen;  //辅助数据的大小
    int msg_flags;     //复制函数中的flags参数，并在调用过程中更新
};
```

msg_iov成员是iovec结构体类型的指针，iovec结构体定义如下：

```C++
struct iovec
{
    void  *iov_base;//内存块起始地址
    size_t  iov_len;//这块内存的长度
};
```


------------------------------------------------------------------


### 14.带外标记

内核通知应用程序带外数据到达的两种常见方式是：I/O复用产生的异常事件和SIGURG信号。

```C++
#include <sys/socket.h>

int  sockatmark(int  sockfd);
```
sockatmark判断sockfd是否处于带外标记，即下一个被读取到的数据是否是带外数据。

如果是，sockatmark返回1，此时我们就可以利用MSG_OOB标志的recv调用来接收带外数据。
如果不是，则sockatmark返回0。



------------------------------------------------------------------



### 15.地址信息函数

在某些情况下，我们想知道一个连接socket的本端socket地址，
以及远端的socket地址。
下面这2个函数正是用于解决这个问题：

```C++
#include <sys/socket.h>

int getsockname(int sockfd, struct sockaddr* address, socklen_t* address_len);
int getpeername(int sockfd, struct sockaddr* address, socklen_t* address_len);
```

getsockname获取sockfd对应的本端socket地址，并将其存储于address参数
指定的内存中，该socket地址的长度则存储于address_len参数指向的变量中。
如果实际socket地址的长度大于address所指内存的大小，
那么该socket地址将被截断。

* getsockname成功时返回0，失败返回-1，并设置errno。

getpeername获取sockfd对应的远端socket地址，
其参数及返回值的含义与getsockname的参数及返回值相同。



------------------------------------------------------------------


### 16.socket选项

如果说fcntl系统调用是控制文件描述符属性的通用POSIX方法，
那么下面两个系统调用则是专门用来读取和设置socket文件描述符属性的方法：

```C++
#include <sys/socket.h>

int getsockopt(int sockfd, int level, int option_name, void* option_value,socklen_t* restrict option_len);
int setsockopt(int sockfd, int level, int option_name, const void* option_value, socklen_t option_len);
```

sockfd参数指定被操作的目标socket。
level参数指定要操作哪个协议的选项（即属性），比如IPv4、IPv6、TCP等。
option_name参数则指定选项的名字。
option_value和option_len参数分别是被操作选项的值和长度。

* getsockopt和setsockopt这两个函数成功时返回0，失败时返回-1并设置errno。

对服务器而言，有部分socket选项要在监听(listen)前针对监听socket设置才有效。
对客户端而言，这些socket选项则应在调用connect函数之前设置，
因为connect调用成功之后，TCP三次握手已完成。



------------------------------------------------------------------


### 17.SO_REUSEADDR选项

服务器程序可以通过设置socket选项SO_REUSEADDR来强制使用
被处于TIME_WAIT状态的连接占用的socket地址。

```C++
//重用本地地址

    const char* ip = argv[1];
    int port = atoi( argv[2] );

    int sock = socket( PF_INET, SOCK_STREAM, 0 );
    assert( sock >= 0 );
    int reuse = 1;
    setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) );

    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );
    int ret = bind( sock, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret != -1 );
```

此外，我们也可以通过修改内核参数 /proc/sys/net/ipv4/tcp_tw_recycle 来
快速回收被关闭的socket，从而使得TCP连接根本不进入 TIME_WAIT状态，
进而允许应用程序立即重用本地的socket地址。



------------------------------------------------------------------


### 18.SO_RCVBUF和SO_SNDBUF选项

SO_RCVBUF和SO_SNDBUF选项分别表示TCP接收缓冲区和发送缓冲区的大小。
不过，当我们用setsockopt来设置TCP的接收缓冲区和发送缓冲区的大小时，
系统都会将其值加倍，并且不得小于某个最小值。

此外，我们可以直接修改内核参数 /proc/sys/net/ipv4/tcp_rmem 和
/proc/sys/net/ipv4/tcp_wmen 来强制
TCP接收缓冲区和发送缓冲区的带下没有最小值限制。



------------------------------------------------------------------


### 19.SO_RCVLOWAT和SO_SNDLOWAT选项

SO_RCVLOWAT和SO_SNDLOWAT选项分别表示TCP接收缓冲区
和发送缓冲区的低水位标记。
它们一般被I/O复用系统调用，用来判断socket是否可读或可写。

默认情况下，TCP接收缓冲区的低水位标记和TCP发送缓冲区的低水位标记均为1字节。



------------------------------------------------------------------

### 20.SO_LINGER选项

SO_LINGER选项用于控制close系统调用在关闭TCP连接时的行为。
默认情况下，当我们使用close系统调用来关闭一个socket时，
close将立即返回，TCP模块负责把该socket对应的TCP发送缓冲区
中残留的数据发送给对方。

```C++
#include <sys/socket.h>

struct linger
{
    int  l_onoff;//开启（非0）还是关闭（0）该选项
	int  l_linger;//滞留时间
};
```



------------------------------------------------------------------

### 21.gethostbyname和gethostbyaddr

gethostbyname 函数根据主机名称获取主机的完整信息，
gethostbyaddr函数根据IP地址获取主机的完整信息。
gethostbyname函数通常先在本地的 /etc/hsots配置的文件中查找主机，
如果没有找到，再去访问DNS服务器。

这两个函数定义如下：
```C++
#include <netdb.h>

struct hostent* gethostbyname(const char* name);
struct hostent* gethostbyaddr(const void* addr, size_t len, int type);
```

hostent结构体定义如下：
```C++
#include <netdb.h>

struct hostent
{
    char* h_name;    //主机名
    char** h_aliases; //主机别名列表，可能有多个
    int h_addrtype;   //地址类型（地址族）
    int h_length;     //地址长度
    char** h_addr_list;//按网络字节序列出的主机IP地址列表
};
```



------------------------------------------------------------------

### 22.getservbyname和getservbyport

getservbyname函数根据名称获取某个服务的完整信息，
getsrvbyport函数根据端口号获取某个服务的完整信息。
他们实际上都是通过读取 /etc/services 文件来获取服务信息的。

```C++
#include <netdb.h>

struct servent* getservbyname(const char* name, const char* proto);
struct servent* getsrvbyport(int port, const char* proto);
```

name参数指定目标服务器的名字，port参数指定目标服务对应的端口号，
proto参数指定服务类型。

结构体servent定义如下：
```C++
#include <netdb.h>

struct servent
{
    char* s_name;       //服务名称
    char** s_aliases;   //服务的别名列表，可能有多个
    int s_port;         //端口号
    char* s_proto;     //服务类型，通常是tcp或者udp
};
```
 
 
------------------------------------------------------------------

### 23.getaddrinfo

getaddrinfo函数既能通过主机名获取ip地址（内部使用gethostbyname）也能
通过服务名获得端口号（内部使用getservbyname）。

```C++
#include <netdb.h>

int getaddrinfo(const char* hostname, const char* service, const struct addrinfo* hints, struct addrinfo** result)
```

hostname参数可以接收主机名，也可以接收字符串表示的IP地址（IPv4用点分十进制
字符串，IPv6用十六进制字符串）。
同样，service参数可以接收服务名，也可以接收字符串表示的十进制端口号。
hints参数是应用程序给getaddrinfo的一个提示，一对getaddrinfo的输出进行更精确的控制。
result参数指向一个链表，该链表用于存储getaddrinfo反馈的结果。

* getaddrinfo成功返回0，失败返回错误码

getaddrinfo反馈的每一条结果都是addrinfo结构体类型的对象，
结构体addrinfo定义如下：

```C++
#include <netdb.h>

struct addrinfo
{
    int ai_flags;  //
    int ai_family;//地址族
    int ai_socktype;//服务类型，SOCK_STREAM 或 SOCK_DGRAM
    int ai_protocol;//
    socklent_t ai_addrlen;// socket地址 ai_addr的长度
    char* ai_canonname;//主机的别名
    struct sockaddr* ai_addr; //指向socket地址
    struct addrinfo* ai_next; //指向下一个sockinfo结构的对象
};
```

```C++
//使用 getaddrinfo 函数
struct  addrinfo  hints;
struct  addrinfo* res;

bzero(&hints, sizeof(hints));
hints.ai_socktype = SOCK_STREAM;
getaddrinfo("ernest-laptop", "daytime", &hints, &res);
```

getaddrinfo将隐式地分配堆内存（可通过valgrind工具查看），
因为res指针原本没有指向一块合法内存的，
所以，getaddrinfo调用结束后，必须使用如下配对函数来释放这块内存：

```C++
#include <netdb.h>

void  freeaddrinfo(struct addrinfo*  res);
```



------------------------------------------------------------------

### 24.getnameinfo

getnameinfo函数能通过socket地址同时获得以字符串表示的主机名（内部使用gethostbyaddr函数）和服务名（内部使用getservbyport函数）。

```C++
#include <netdb.h>

int getnameinfo(const struct sockaddr* sockaddr, socklen_t addrlen, char* host, socklen_t hostlen, char* serv, socklen_t servlen, int flags);
```

getnameinfo将返回的主机名存储在host参数指向的缓存中，
将服务名存储在serv参数指向的缓存中，
hostlen和servlen参数分别指定这两块缓存的长度。
flags参数控制getnameinfo的行为。

* getnameinfo成功返回0，失败返回错误码


------------------------------------------------------------------


### 25.错误码

Linux下strerror函数能将数值错误码errno转换成易读的字符串形式。
同样，下面的函数可将表5-8(getaddrinfo和getnameinfo的错误码)的错误码转换成其字符串形式：

```C++
#include <netdb.h>

const char* gai_strerror(int error);
```











