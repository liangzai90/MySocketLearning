/***********************
《Linux高性能服务器编程》游双  著

服务器 监听12345端口，启动服务器的时候是要带参数的 ./a.out 192.168.12.10 12345 2 

客户端，可以用 telnet 192.168.12.10 12345 来链接这个端口
然后在 服务器 查看端口的链接情况 netstat  -nt | grep 12345

服务器接收3个参数：IP地址，端口号和backlog值。在客户端多次telnet，可以发现只有（backlog+1）3个是建立连接的，其他都是SYN_RECV状态。

代码清单5-3：testlisten
*******************************/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool stop = false;

/* SIGTERM 信号的处理函数，触发时结束主程序中的循环 */
static void handle_term(int sig)
{
	stop = true;
}

int main(int argc, char* argv[])
{
	signal(SIGTERM, handle_term);
	if (argc <= 3)
	{
		printf("usage: %s ip_address port_number backlog.\r\n", basename(argv[0]));
		return 1;
	}

	const char* ip = argv[1];
	int port = atoi(argv[2]);
	int backlog = atoi(argv[3]);

	int sock = socket(PF_INET, SOCK_STREAM, 0);
	assert(socket >= 0);

	// create a ipv4 socket address
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);

	int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
	assert(ret != -1);

	ret = listen(sock, backlog);
	assert(ret != -1);


	// wait for SIGTERM
	while (!stop)
	{
		sleep(1);
	}

	// close socket
	close(sock);

	printf("hello socket...");
	return 0;
}

