#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define BUFF_SIZE  1024

int main(int argc, char* argv[])
{
    if(argc <= 2)
    {
        printf("usage:%s ip_address, port_number.\r\n", basename(argv[0]));
    }   
    
    const char* ip = argv[1];
    int port = atoi(argv[2]);//char to int
    
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    
    address.sin_family = AF_INET;
    address.sin_port = htons(port);//little to big
    inet_pton(AF_INET, ip, &address.sin_addr);
    
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >=0);
    
    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret >=0);
    
    ret = listen(sock, 5);
    assert(ret >=0);
    
    struct sockaddr_in clientsock;
    socklen_t client_addrlength = sizeof(clientsock);
    int connfd = accept(sock, (struct sockaddr*)&clientsock, &client_addrlength);
    if(connfd < 0)
    {
        printf("errno is:%d.\r\n", errno);
    }
    else
    {
        char buffer[BUFF_SIZE];
        memset(buffer,'\0',sizeof(buffer));
        ret = recv(connfd, buffer, BUFF_SIZE - 1,0);
        printf("got %d bytes of normal data %s \r\n", ret, buffer);
        
        memset(buffer,'\0',sizeof(buffer));
        ret = recv(connfd, buffer, BUFF_SIZE - 1,MSG_OOB);
        printf("got %d bytes of oob data %s \r\n", ret, buffer);
        
        memset(buffer,'\0',sizeof(buffer));
        ret = recv(connfd, buffer, BUFF_SIZE - 1,0);
        printf("got %d bytes of normal data %s \r\n", ret, buffer);
        
        close(connfd);        
    }
    
    close(sock);

    return 0;
}
