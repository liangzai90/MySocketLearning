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






