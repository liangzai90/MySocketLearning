
# 第6章  高级I/O函数

Linux提供了很多高级的I/O函数。它们并不像Linux基础I/O函数（比如open和read）
那么常用（编写内核模块时，一般要实现这些I/O函数），但在特定的条件下
却表现出优秀的性能。本章将讨论其中和网络编程相关的几个，
这些函数大致分为三类：

- [ ] 用于创建文件描述符的函数，包括pipe、dup、dup2函数。

- [ ] 用于读写数据的函数，包括readv/writev、sendfile、mmap/munmap、splice和tee函数。

- [ ] 用于控制I/O行为和属性的函数，包括fcntl函数。


------------------------------------------------------------------


### 1.pip函数

pip函数可用于创建一个管道，以实现进程间通信。

```C++
#include <unistd.h>

int  pipe(int  fd[2]);
```

pip函数的参数是一个包含两个int型整数的数组指针。
该函数成功时返回0，并将一对打开的文件描述符值填入其参数指向的数组。

* 如果失败，则返回-1并设置errno。

**fd[0]**只能用于从管道**读出数据**

**fd[1]**只能用于从管道**写入数据**

此外，socket的基础API中有一个socketpair函数。能够方便的创建双向管道。
其定义如下：
```C++ 
#include <sys/types.h>
#include <sys/socket.h>

int  socketpair(int domain, int type, int protocol, int fd[2]);
```

socketpair前三个参数的含义与socket系统调用的三个参数完全相同。
但是domain**只能使用UNIX本地域协议族AF_NUIX**，因为
我们仅仅能在本地使用这个双向管道。
最后一个参数则和pipe系统调用的参数一样，只不过socketpair创建的
这对文件描述符都是即可读又可写的。

* socketpair成功时返回0，失败时返回-1并设置errno。


------------------------------------------------------------------


### 2.dup函数和dup2函数

有时候我们希望把标准输入重定向到一个文件，或者把标准输出重定向到
一个网络连接（比如CGI编程）。
这个可以通过下面的用于复制文件描述符的dup或dup2函数来实现。

```C++
#include <unistd.h>

int dup(int file_descriptor);
int dup2(int file_descriptor_one, int file_descriptor_two);
```
dup函数创建一个新的文件描述符，该新文件描述符和原有文件描述符file_descriptor
指向相同的文件、管道或者网络连接。
dup返回的文件描述符总是取系统当前可用的最小整数值。
dup2和dup类似，不过它将返回第一个不小于file_descriptor_two的整数值。

* dup和dup2系统调用失败时返回-1，并设置errno。

通过dup和dup2创建的文件描述符并不继承原文件描述符的属性，
比如close-on-exec和non-blocking等。



------------------------------------------------------------------

### 3.readv函数和writev函数

readv函数将数据从文件描述符读到分散的内存块中，即分散读；

writev函数则将多块分散的内存数据一并写入文件描述符中，即集中写。

```C++
#include <sys/uio.h>

ssize_t  readv(int fd, const struct iovec* vector, int count);
ssize_t  writev(int fd, const struct iovec* vector, int count);
```
fd参数是被操作的目标文件描述符。
vector参数的类型是iovec结构体。
coutnt参数是vector数组的长度，即有多少块内存数据需要从fd读出或写到fd。

* readv/writev在成功时返回读出/写入fd的字节数，失败则返回-1并设置errno。



------------------------------------------------------------------

### 4.sendfile函数

sendfile函数在两个文件描述符之间直接传递数据（完全在内核中操作），从而避免了
内核缓冲区和用户缓冲区之间的数据拷贝，效率很高，这被称为**零拷贝**。
sendfile函数定义如下：
```C++
#include <sys/sendfile.h>

ssize_t  sendfile(int out_fd, int in_fd, off_t* offset, size_t count);
```

in_fd参数是待读出内容的文件描述符，out_fd参数是写入内容的文件描述符。
offset参数指定从读入文件流的哪个位置开始读，如果为空，则使用读入文件流默认的起始位置。
count参数指定在文件描述符in_fd和out_fd之间传输的字节数。

* sendfile成功时返回传输的字节数，失败则返回-1并设置errno。

in_fd必须是一个支持类似mmap函数的文件描述符，即它必须指向真是的文件，不能是socket和管道。

out_fd则必须是一个socket。


------------------------------------------------------------------

### 5.mmap函数和munmap函数

mmap函数用于申请一端内存空间。我们可以将这段内存作为进城间通信的共享内存，
也可以将文件直接映射到其中。
munmap函数则释放由mmap创建的这段内存空间。
它们的定义如下：

```C++
#include <sys/mman.h>

void*  mmap(void *start, size_t length, int port, int flags, int fd, off_t offset);
int  munmap(void *start, size_t length);
```

start参数允许用户使用某个特定的地址作为这段内存的起始地址。
如果被设置为NULL，则系统自动分配一个地址。
length参数指定内存段的长度。
port参数用来设置内存段的访问权限。
flags参数控制内存段内容被修改后程序的行为。
fd参数是被映射文件对应的文件描述符（一般通过open系统调用获得）。
offset参数设置从文件的何处开始映射

* mmap函数成功时返回指向目标内存区域的指针，失败则返回MAP_FAILED，并设置errno。

* munmap函数成功时返回0，失败则返回-1并设置errno。



--------------------------------------------------------------------

### 6.splice函数

splice函数用于在两个文件描述符之间移动数据，也是**零拷贝操作**。
splice函数定义如下：

```C++
#include <fcntl.h>

ssize_t  splice(int  fd_in, loff_t*  off_in, int fd_out, loff_t* off_out, size_t len, unsigned int flags);
```

fd_in参数是待输入数据的文件描述符。
如果fd_in是一个管道文件描述符，那么off_in参数必须被设置为NULL。
如果fd_in不是一个管道文件描述符（比如socket），那么off_in表示
从输入数据流的何处开始读取数据。

使用splice函数时，fd_in和fd_out必须至少有一个是管道文件描述符。

*调用成功返回移动字节数量，可能返回0表示没有数据需要移动。失败时返回-1并设置errno。


-------------------------------------------------------------------

### 7.tee函数

tee函数在两个管道文件描述符之间复制数据，也是**零拷贝操作**。
它不消耗数据，因此源文件描述符上的数据仍然可用于后续的读操作。
tee函数的定义如下：

```C++
#include <fcntl.h>

ssize_t  tee(int fd_in, int fd_out, size_t  len, unsigned int flags);
```

该函数的参数的含义与splice相同（但fd_in和fd_out必须都是管道文件描述符）。
tee函数成功时返回在两个文件描述符之间复制的数据量（字节数）。返回0表示没有复制任何数据。

* tee失败则返回-1，并设置errno。


-------------------------------------------------------------------

### 8.fcntl函数

fcntl函数，正如其名字（file control）描述的那样，提供了对文件描述符的各种控制操作。
另外一个常见的控制文件描述符属性和行为的系统调用是**ioctl**。

```C++
#include <fcntl.h>

int  fcntl(int fd, int cmd, ...);
```

fd参数是被操作的文件描述符，
cmd参数指定执行何种类型的操作。

* fcntl函数成功时的返回值如表所示。失败则返回-1并设置errno。

```C++
//将文件描述符设置为非阻塞的
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);//获取文件描述符旧的状态标志
	int new_optin = old_option | O_NONBLOCK;//设置非阻塞标志
	fcntl(fd, F_SETFL, new_option);
	return old_option;   //返回文件描述符旧的状态标志，以便日后恢复该状态标志
}
```
