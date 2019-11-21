#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE  512

int main(int argc, char* argv[])
{
    if(argc<=2)
    {
        printf("usage: %s ip_address, port_number send_buffer_size \r\n", basename(argv[0]));
        return 1;
    }
    
    char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in serverSocket;
    bzero(&serverSocket, sizeof(serverSocket));

    serverSocket.sin_port = htons(port);
    serverSocket.sin_family = AF_INET;
    inet_pton(AF_INET, ip , &serverSocket.sin_addr);
    
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock>=0);
    
    int sendBuf =atoi(argv[3]);
    int len = sizeof(sendBuf);
    //set tcp buff ,and read
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendBuf, sizeof(sendBuf));
    getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendBuf, (socklen_t*)&len);
    printf("the tcp send buffer size after setting is %d.\r\n", sendBuf);
    
    if(connect(sock, (struct sockaddr*)&serverSocket, sizeof(serverSocket)) != -1)
    {
        char buffer[BUFFER_SIZE];
        memset(buffer, '\0', sizeof(buffer));
        send(sock, buffer, BUFFER_SIZE, 0);
        
        
        
        //client get server(peer) name
        struct sockaddr_in peerHost;
        socklen_t client_addrlength = sizeof(peerHost);
        getpeername(sock, (struct sockaddr*)&peerHost, &client_addrlength );
        char dest[100] ;
        inet_ntop(AF_INET, &peerHost.sin_addr,dest,100);
        int peerPort=ntohs(peerHost.sin_port);
        printf("dest ip : %s, port:%d", dest, peerPort);
    }
    
    close(sock);    

    return 0;
}
