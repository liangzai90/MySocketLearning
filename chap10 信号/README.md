# 第10章  信号

信号是由用户、系统或者进程发送给目标进程的信息，以通知目标进程某个状态的改变或系统异常。
Linux信号可由如下条件产生：

- [ ] 对于前台进程。用户可以通过输入特殊的终端字符来给它发送信号。
比如输入 Ctrl+C 通常会给进程发送一个终端信号。

- [ ] 系统异常。比如浮点异常和非法内存段访问。

- [ ] 系统状态变化。比如alarm定时器到期引起的SIGALRM信号。

- [ ] 运行kill命令或调用kill函数

服务器程序必须处理（或至少忽略）一些常见的信号，以避免异常终止。

本章先讨论如何在程序中发送信号和处理信号，然后讨论Linux支持的信号种类，
并详细探讨其中和网络编程密切相关的几个。



--------------------------------------------------------------------------------

## 10.1 Linux信号概述

### 10.1.1 发送信号

Linux下，一个进程给其他进程发送信号的API是kill函数。
其定义如下：

```C++
#include  <sys/types.h>
#include  <signal.h>

int kill(pid_t pid, int sig);
```

该函数把信号sig发送给目标进程；
目标进程由pid参数指定。


### 10.1.2 信号处理方式

目标进程在收到信号时，需要定义一个接收函数来处理。
信号处理函数的原型如下：

```C++
#include  <signal.h>

typedef  void (*__sighandler_t)  ( int );
```

信号处理函数只带有一个整形参数，该参数用来指示信号类型。

除了用户自定义信号处理函数外，bits/signum.h 头文件中还定义了信号的
两种其他处理方式--SIG_IGN和SIG_DEL：

```C++
#include <bits.signum.h>

#define  SIG_DFL  ((__sighandler_t)  0)
#define  SIG_IGN  ((__sighandler_t)  1)
```

SIG_IGN表示忽略目标信号，
SIG_DFL表示信号的默认处理方式

信号的默认处理方式有：
* 结束进程（Term）
* 忽略信号（Ign）
* 结束进程并生成核心转储文件（Core）
* 暂停进程（Stop）
* 继续进程（Cont）

### 10.1.3 Linux信号

### 10.1.4 中断系统调用

  如果程序在执行处于阻塞状态的系统调用时接收到信号，并且我们为该信号设置了信号处理函数，
  则默认情况下系统调用将被中断，并且errno被设置为EINTR。
  我们可以使用sigaction函数为信号设置SA_RESTART标志以
  自动重启该被信号中断的系统调用。
  
## 10.2 信号函数
  
### 10.2.1 signal 系统调用

要为一个信号设置处理函数，可以使用下面的signal系统调用：

```C++
#include  <signal.h>

_sighandler_t  signal( int sig, _sighandler_t  _handler )
```

sig参数支出要捕获的信号类型。
_handler参数是 _sighandler_t类型的函数指针，用于指定信号sig的处理函数

* signal函数成功时返回一个函数指针，该函数指针的类型也是_sighandler_t。
* signal系统调用出错时返回SIG_ERR，并设置errno。

### 10.2.2 sigaction 系统调用

设置信号处理函数的更健壮的接口是如下的系统调用：

```C++
#include  <signal.h>

int sigaction(int sig, const struct sigaction* act, struct sigaction* oact);
```

sig参数支出要捕获的信号类型，
act参数指定新的信号处理方式，
oact参数则输出信号先前的处理方式

```C++
struct  sigaction
{
#ifdef  __USE_POSIX199309
    union
    {
      _sighandler_t  sa_handler;
      void (*sa_sigaction) (int, siginfo_t*, void*);
    }_sigaction_handler;
#define sa_handler __sigaction_handler.sa_handler
#define sa_sigaction __sigaction_handler.sa_sigaction
#else
  _sighandler_t sa_handler;
#endif
  _sigset_t sa_mask;
  int sa_flags;
  void (*sa_restorer) (void)
};
```
该结构体中，sa_hander成员指定信号处理函数。
sa_mask成员设置进程的信号掩码，以指定哪些信号不能发送给本进程。
sa_mask是信号集 sigset_t类型，该类型指定一组信号。
sa_flags成员用于设置程序收到信号时的行为。

* sigaction成功时返回0，失败则返回-1，并设置errno。


------------------------------------------------------------------------

## 10.3 信号集

### 10.3.1 信号集函数

Linux使用数据结构sigset_t来表示一组信号。其定义如下：

```C++
#include  <bits/sigset.h>

#define  _SIGSET_NWORDS (1024 / (8 * sizeof(unsigned long int)))
typedef struct
{
  unsigned long int __val[SIGSET_NWORDS];
}__sigset_t;
```

由该定义可见，sigset_t实际上是一个长整型数组，数组的每个元素的每个位表示一个信号。
Linux提供了如下一组函数来设置、修改、删除和查询信号集：

```C++
#include <signal.h>

int sigemptyset(sigset_t* _set);//清空信号集
int sigfillset(sigset_t* _set);//在信号集中设置所有信号
int sigaddset(sigset_t* set, int _signo;//将信号 _signo 添加至信号集中国
int sigdelset(sigset_t* set, int _signo;//将信号 _signo 从信号集中删除
int sigismember(const sigset_t* set, int _signo;//测试 _signo 是否在信号集中 
```

### 10.3.2 进程信号掩码

可以利用 sigaction 结构体的 sa_mask 成员来设置进程的信号掩码。
如下函数也可以用来设置或查看进程的信号掩码：

```C++
#include <signal.h>

int sigprocmask(int _how, _const sigset_t* _set, sigset_t* _oset);
```

_set参数指定新的信号掩码，_oset参数则输出原来的信号掩码。
如果 _set参数不为NULL，则_how参数指定设置进程信号掩码的方式。
如果 _set为NULL，则信号掩码不变，此时我们可以根据 _oset参数来得进程当前的信号掩码。

* sigprocmask成功时返回0，失败则返回-1并设置errno。


### 10.3.3 被挂起的信号

设置信号掩码之后，被屏蔽的信号将不能被进程接收就。
如果给进程发送一个被屏蔽的信号，则操作系统将该信号设置为进程的一个被挂起的信号。
如果我们取消对被挂起信号的屏蔽，则它能立即被进程接收到。

如下函数可以获得进程当前被挂起的信号集：

```C++
#include <signal.h>

int sigpending(sigset_t* set);
```
set参数用于保存被挂起的信号集。

* sigpending函数成功时返回0，失败则返回-1并设置errno。


-------------------------------------------------------------------------------

## 10.4 统一事件源

信号是一种异步事件：信号处理函数和程序的主循环是两条不同的执行路线。

    把信号的主要处理逻辑放到程序的主循环中，当信号处理函数被触发时，它只是简单地通知主循环程序接收到信号，
    并把信号值传递给主循环，主循环再跟进接收到的信号值执行目标信号对应的逻辑代码。
    信号处理函数通常管道来将信号“传递”给主循环：信号处理函数往管道的写端写入信号值，
    主循环则从管道的读端读出该信号值。
    那么主循环怎么知道管道上何时有数据可读呢？我们只需要使用I/O复用系统调用来监听
    管道的读端文件描述符上的可读事件。如此一来，信号事件就能和其他I/O事件一样被处理，
    即统一事件源。

很多优秀的I/O框架和后台服务器程序都统一处理信号和I/O事件，比如Libevent I/O框架库和xinetd超级服务。


-----------------------------------------------------------------------

## 10.5 网络编程相关信号

### 10.5.1 SIGHUP

当挂起进程的控制终端时，SIGHUP信号将被触发。对于没有控制终端的网络后台程序而言，它们通常利用SIGUP信号来强制服务器重读配置文件。

### 10.5.2 SIGPIPE

默认情况下，往一个读端关闭的管道或socket连接中写数据将引发 SIGPIPE 信号。

我们可以利用I/O复用系统调用来检测管道和socket连接的读端是否已经关闭。
以poll为例，当管道的读端关闭时，写端文件描述符上的POLLHUP事件将被触发；
当socket连接被对方关闭时，socket上的POLLRDHUP事件将被触发。

### 10.5.3 SIGURG

在Linux环境下，内核通知应用程序带外数据到达主要有两种方式：一种是第9章介绍的I/O复用技术，
select等系统调用在接收到带外数据时将返回，并向应用程序报告socket上的异常事件；
另一种方法就是使用SIGURG信号。




