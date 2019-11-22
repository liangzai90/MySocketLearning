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

int  inet_pton(int af, const char* src, void* src);
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


```C++

```


```C++

```


```C++

```


```C++

```


```C++

```


```C++

```


```C++

```
















