#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main(int argc, char* argv[])
{
    if(argc<=2)
    {
        printf("suage:%s IP_number port_number recv_buffer.\r\n", basename(argv[0]));
        return 1;
    }

    char* ip = argv[1];
    int port = atoi(argv[2]);
    
    struct sockaddr_in serverAddr;
    bzero(&serverAddr,sizeof(serverAddr));
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET,ip, &serverAddr.sin_addr);
    
    int sockfd = socket(PF_INET, SOCK_STREAM,0);
    assert(sockfd>=0);
    
    int recvbuf = atoi(argv[3]);
    int len = sizeof(recvbuf);
    //set tcp bufsize,then read it.
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvbuf, sizeof(recvbuf));
    getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvbuf, (socklen_t*)&len);
    printf("The tcp receive buffer size after setting is %d.\r\n", recvbuf);

    int ret = bind(sockfd, (struct sockaddr*)&serverAddr,sizeof(serverAddr));
    assert(ret>=0);
    
    ret = listen(sockfd, 5);
    assert(ret>=0);

    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sockfd, (struct sockaddr*)&client, &client_addrlength);
    if(connfd < 0)
    {
        printf("errno is:%d \r\n", errno);
    }
    else
    {
        char buffer[BUFFER_SIZE];
        memset(buffer,'\0', sizeof(buffer));
        while(recv(connfd, buffer, BUFFER_SIZE-1, 0) > 0)
        {
           printf("recv success...");
        }
        
        close(connfd);
    }    
    
    close(sockfd);

    return 0;
}
