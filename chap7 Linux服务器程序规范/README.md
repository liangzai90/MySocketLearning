

# 第7章 Linux服务器程序规范

除了网络通信外，服务器程序通常还必须考虑许多其他细节问题。
这些细节问题涉及面广且零碎，而且基本上是模板式的，
所以我们称之为服务器程序规范。比如：

- [ ] Linux服务器程序一般以后台进程形式运行。后台进程又称守护进程（daemon）。
它没有控制终端，因而也不会意外接收到用户输入。
守护进程的父进程通常是init进程（PID为1的进程）。

- [ ] Linux服务器程序通常有一套日志系统，它至少能输出日志到文件，
有的高级服务器还能输出日志到专门的UDP服务器。
大部分后台进程都在 /var/log 目录下拥有自己的日志目录。

- [ ] Linux服务器程序一般以某个专门的非root身份运行。
比如mysqld、httpd、syslogd等后台进程，分别拥有自己的运行账户mysql、apache和syslog。

- [ ] Linux服务器程序通常是可配置的。
服务器程序通常能处理很多命令行选项，如果一次运行的选项太多，则可以用配置文件来管理。
绝大多数服务器程序都有配置文件，并存放在 /etc 目录下。
比如第4章讨论的squid服务器的配置文件是 /etc/squid/squid.conf

- [ ] Linux服务器进程通常会在启动的时候生成一个PID文件并存入 /var/run 目录中，
以记录该后台进程的PID。比如syslog的PID文件是 /var/run/syslogd.pid。

- [ ] Linux服务器程序通常需要**考虑系统资源和限制**，
以预测自身能承受多大负荷，比如进程可用文件描述符总数和内存总量等。

在开始系统地学习网络编程之前，我们将用一章的篇幅来探讨服务器程序的一些主要的规范。


-------------------------------------------------------------------

### 1.Linux系统日志

Linux提供一个守护进程来处理系统日志----syslogd，不过现在的Linux系统上使用的是
它的升级版本----rsyslogd。

内核日志由printk等函数打印至内核的环状缓存（ring buffer）中。

环状缓存（ring buffer）的内容直接映射到 /proc/kmsg文件中。

rsyslogd则通过读取该文件（/proc/kmsg文件）获得内核日志。

默认情况下，
* 调试信息保存至 /var/log/debug文件中
* 普通信息保存至 /var/log/messages文件中
* 内核消息保存至 /var/log/kern.log文件中

rsyslogd的主配置文件是 /etc/rsyslog.conf


-------------------------------------------------------------------

### 2.syslog函数

应用程序使用syslog函数与rsyslogd守护进程通信。
syslog函数的定义如下：

```C++
#include <syslog.h>

void syslog(int priority, const char* message, ...);
```

该函数采用可变参数来结构化输出。
priority参数是所谓的设施值与日志级别的按位或。
设施值的默认值是LOG_USER。
日志级别有如下几个：
```C++
#include <syslog.h>

#define  LOG_EMERG      0   //系统不可用
#define  LOG_ALERT      1   //报警，需要立即采取行动
#define  LOGCRIT        2   //非常严重的情况
#define  LOG_ERR        3   //错误
#define  LOG_WARNING    4   //警告
#define  LOG_NOTICE     5   //通知
#define  LOG_INFO       6   //信息
#define  LOG_DEBUG      7   //调试
```

下面这个函数可以改变syslog的默认输出方式，进一步结构化日志内容：

```C++
#include <syslog.h>

void  openlog(const char* ident, int logopt, int facility);
```

ident参数指定的字符串将被添加到日志消息的日期和时间之后，它通常被设置为程序的名字。
logopt参数对后续syslog调用的行为进行配置。
```C++
#define  LOG_PID      0x01  //在日志消息中包含程序pid
#define  LOG_CONS     0x02  //如果消息不能记录到日志文件，则打印至终端
#define  LOG_ODELAY   0x04  //延迟打开日志功能，直到第一次调用syslog
#define  LOG_NEDLAY   0x08  //不延迟打开日志功能
```

facility参数可用来修改syslog函数中的默认设置值。

**日志过滤**

可以通过简单地设置日志掩码，是日志级别大于日志掩码的日志信息被系统忽略。

下面这个函数用于设置syslog的日志掩码：

```C++
#include <syslog.h>

int  setlogmask(int maskpri);
```

maskpri参数指定日志掩码值。该函数始终会成功，它返回调用进程先前的日志掩码值，
最后，**不要忘了使用如下的函数关闭日志功能**

```C++
#include <syslog.h>

void  closelog();
```




-------------------------------------------------------------------

### 3.UID、EUID、GID和EGID

用户信息对于服务器程序的安全性来说是很重要的，比如大部分服务器就必须
以root身份启动，但不能以root身份运行。

下面这一组函数可以获取和设置当前进程的
真实用户ID（UID）、有效用户ID（EUID）、真实组ID（GID）和
有效组ID（EGID）：

```C++
#include <sys/types.h>
#include <unistd.h>

uid_t  getuid();         //获取真实用户ID
uid_t  geteuid();        //获取有效用户ID
gid_t  getgid();        //获取真实组ID
gid_t  getegid();        //获取有效组ID

int setuid(uid_t uid);    //设置真实用户ID
int seteuid(uid_t uid);   //设置有效用户ID
int setgid(gid_t gid);    //设置真实组ID
int setegid(gid_t gid);    //设置有效组ID
```

一个进程拥有两个用户ID：UID和EUID。
EUID存在的目的是方便资源访问：它使得运行程序的用户拥有该程序的有效用户的权限。



-------------------------------------------------------------------

### 4.进程组

Linux下每个进程都隶属于一个进程组，因此它们除了PID信息外，还有进程组ID(PGID)。
我们可以用如下函数来获取指定进程的PGID:

```C++
#include <unistd.h>

pid_t  getpgid(pid_t pid);
```
该函数成功时返回进程pid所属进程组的PGID，失败则返回-1并设置errno。

设置PGID:

```C++
#include <unistd.h>

int setpgid(pid_t pid, pid_t pgid);
```

该函数将PID为pid的进程的PGID设置为pgid。
如果pid和pgid相同，则由pid指定的进程将被设置为进程组首领；

* setpgid函数成功时返回0，失败则返回-1并设置errno。

一个进程只能设置自己活其子进程的PGID。
并且，当子进程调用exec系列函数后，我们也不能再在父进程中对它设置PGID。



-------------------------------------------------------------------

### 5.会话

一些有关联的进程组将形成一个会话（session）。
下面的函数用于创建一个会话：

```C++
#include <unistd.h>

pid_t  setsid(void);
```
该函数不能由进程组的首领进程调用，否则将产生一个错误。

* 成功时返回新的进程组的PGID，失败则返回-1并设置errno。

Linux并未提供所谓会话ID(SID)的 概念，但Linux系统认为它等于会话
首领所在的进程组的PGID，并提供了如下函数来读取SID:

```C++
#include <unistd.h>

pid_t  getsid(pid_t  pid);
```


-------------------------------------------------------------------

### 6.系统资源限制

Linux上运行的程序都会受到资源限制的影响，比如物理设备限制（cpu数量，内存数量等）、
系统策略限制（CPU时间等），以及具体实现的限制（比如文件名的最大长度）。
Linux系统资源限制可以通过如下一对函数来读取和设置：

```C++
#include <sys/resource.h>

int getrlimit(int resource, struct rlimit *rlim);
int setrlimit(int resource, const struct rlimit *rlim);
```

rlim参数是rlimit结构体类型的指针，rlimit结构体定义如下：

```C++
struct rlimit
{
    //rlim_t是一个整数类型，它描述资源级别
    rlim_t  rlim_cur;//软限制
	rlim_t  rlim_max;//硬限制
};
```



-------------------------------------------------------------------

### 7.改变工作目录和根目录

获取进程当前工作目录和改变进程工作目录的函数分别是：

```C++
#include <unistd.h>

char*  getcwd(char* buf, size_t size);
int  chdir(const char* path);
```

buf 参数指向的内存用于存储进程当前工作目录的绝对路径名，其大小由size参数决定。

* getcwd函数成功时返回一个指向目标存储区的指针，失败则返回NULL并设置errno。

chdir函数的path参数指定要切换到的目标目录。
* 它成功时返回0，失败时返回-1并设置errno。


改变根目录的函数是chroot，其定义如下：

```C++
#include <unistd.h>

int  chroot(const char* path);
```

path参数指定要切换到的目标根目录。

chroot并不改变进程的当前工作目录。

* 它成功时返回0，失败则返回-1并设置errno。


### 8.服务器程序后台化

    将服务器以守护进程的方式运行
    请参考例子chap7.3 

	
Linux提供了完成同样功能的库函数

```C++
#include <unistd.h>

int daemon(int nochdir, int noclose);
```

nochidr参数用于指定是否改变工作目录，如果给它传递0，
则工作目录将被设置为"/"(根目录)，否则继续使用当前工作目录。

noclose参数为0时，标准输入、标准输出和标准错误输出都被重定向到/dev/null文件，
否则依然使用原来的设备。

* 该函数成功时返回0，失败则返回-1,并设置errno。





