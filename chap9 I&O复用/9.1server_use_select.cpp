//
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    if(argc<=2)
    {
        printf("usage: %s IP_address Port_number \r\n", basename(argv[0]));
        return 1;
    }
    
    char* ip = argv[1];
    int port = atoi(argv[2]);
    
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);
    
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >=0);
    
    int ret = bind(sockfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);
    
    ret = listen(sockfd, 5);
    assert(ret != -1);
    
    struct sockaddr_in client_addr;
    socklen_t client_addrlength = sizeof(client_addr);
    int connfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addrlength);
    if(connfd < 0)
    {
        printf("connection failed.errno is :%d \r\n", errno);
        close(sockfd);
    }
    
    //////
    char buf[1024];
    fd_set read_fds;
    fd_set exception_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&exception_fds);
    
    while(1)
    {
        memset(buf, '\0', sizeof(buf));
        //每次调用select前都要重新再read_fds和exception_fds中
		//设置文件描述符connfd，
		//因为事件发生之后，文件描述符集合将被内核修改
        FD_SET(connfd, &read_fds);
        FD_SET(connfd, &exception_fds);
        ret = select(connfd+1, &read_fds, NULL, &exception_fds, NULL);
        if(ret < 0)
        {
            printf("selection failure.\r\n");
            break;
        }
        
        //对于可读事件，采用普通的recv函数读取数据
        if(FD_ISSET(connfd, &read_fds))
        {
            ret = recv(connfd, buf, sizeof(buf)-1, 0);
            if(ret<=0)
            {
                break;
            }
            printf("get %d bytes of normal data:%s \r\n", ret, buf);
        }
		//对于异常事件，采用带外MSG_OOB标志的 recv 函数读取带外数据
        else if(FD_ISSET(connfd, &exception_fds))
        {
            ret = recv(connfd, buf, sizeof(buf)-1, MSG_OOB);
            if(ret <= 0)
            {
                break;
            }
            printf("get %d bytes of oob data: %s \r\n", ret ,buf);
        }        
    } 
    
    close(connfd);    
    close(sockfd);
    
    return 0;
}


