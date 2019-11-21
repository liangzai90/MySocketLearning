/***********************
《Linux高性能服务器编程》游双  著

代码清单5-5：接受一个异常的连接

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
#include <errno.h>

int main(int argc, char* argv[])
{
	if (argc <= 2)
	{
		printf("usage: %s ip_address port_number.\r\n", basename(argv[0]));
		return 1;
	}

	const char* ip = argv[1];
	int port = atoi(argv[2]);

	// create a ipv4 socket address
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);

	int sock = socket(PF_INET, SOCK_STREAM, 0);
	assert(sock >= 0);

	int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
	assert(ret != -1);

	ret = listen(sock, 2);
	assert(ret != -1);

	//暂停10秒，以等待客户端连接和相关操作（掉线或者退出）完成
	printf("accept a client... I sleep for a while.\r\n");
	sleep(10);

	struct sockaddr_in client;
	socklen_t client_addrlength = sizeof(client);
	int connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);
	if (connfd < 0)
	{
		printf("errno is:%d", errno);
	}
	else
	{
		//接受连接成功，则打印客户端的IP地址和端口号
		char remote[INET_ADDRSTRLEN];
		printf("connected with IP:%s and port: %d.\r\n", inet_ntop(AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN), ntohs(client.sin_port));
		close(connfd);
	}


	// close socket
	close(sock);

	printf("hello socket...sock closed...");

	return 0;
}

