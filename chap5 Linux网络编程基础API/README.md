# 第5章 Linux网络编程基础API

探讨Linux网络编程基础API与内核中TCP/IP协议族之间的关系，并未后续章节提供编程基础。从3个方面讨论Linux网络API.

- [ ] socket地址API。socket 最开始的含义是一个IP地址和端口对（ip, port）。它唯一地表示了使用TCP通信的一端。本书称其为socket地址。

- [ ] socket基础API。socket的主要API都定义在 sys/socket.h 头文件中，包括创建socket、命名socket、监听socket、接受连接、发起连接、读写数据、
获取地址信息、检测带外标记，以及读取和设置socket选项。

- [ ] 网络信息API。Linux 提供了一套网络信息API，以实现主机名和IP地址之间的转换，以及服务名称和端口号之间的转换。
这些API都定义在 netdb.h 头文件中，我们将讨论其中几个主要的函数。

-----------------------------------------------------------------


### 1.主机字节序和网络字节序

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

大端字节序，也称网络字节序。网络上传输的数据，都是网络字节序的。

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
* [C/C++](#-cc)

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
在实际使用时都要转化为通用socket地址类型**sockaddr**（强制转换即可），
因为 **所有socket编程接口使用的地址参数的类型都是 sockaddr **。


# 1


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

失败时返回INADDR_NONE。

**inet_aton**函数完成和inet_addr同样的功能，但是将转化结果存储于参数inp指向的地址结构中。

成功时返回1，失败则返回0。

**inet_ntoa**函数将用网络字节序整数表示的IPv4地址转化为用点分十进制字符串表示的IPv4地址。
**inet_ntoa不可重入，非线程安全**，该函数内部用一个静态变量存储转化结果，
函数返回值指向该静态内存。



**同时适用于IPv4和IPv6地址**
```C++
#include <arpa/inet.h>
int  inet_pton(int af, const char* src, void* src);
const char* inet_ntop(int af, const void* src, char* dst, socklen_t cnt);
```
**inet_pton**函数将用字符串表示的IP地址src（用点分十进制字符串表示的IPv4地址
或用十六进制字符串表示的IPv6地址）转换成用网络字节序整数表示的IP地址，并把转换结果存储于dst
指向的内存中。
其中，af参数指定地址族，可以使AF_INET或者AF_INET6.

inet_pton成功时返回1，失败则返回0并设置errno。

**inet_ntop**函数进行相反的转换，前3个参数的含义与inet_pton的参数相同，最后一个参数cnt
指定目标存储单元大小。下面的2个宏可以帮助我们指定这个大小（分别用于IPv4和IPv6）

```C++
#include <netinet/in.h>
#define  INET_ADDRSTRLEN   16
#define  INET6_ADDRSTRLEN  46
```
inet_ntop成功时返回目标存储单元的地址，失败则返回NULL并设置errno。

## C/C++


:smile: # 1


**举个IP地址转换的例子**
```C++
    address.sin_port = htons(port);//little to big
    inet_pton(AF_INET, ip, &address.sin_addr);
    
    char dest[100] ;
    inet_ntop(AF_INET, &peerHost.sin_addr,dest,100);

```

-----------------------------------------------------------------

### 5.创建socket

:smile: /github-flavored-markdown # 1

Any word wrapped with two tildes (like ~~this~~) will appear crossed out.

