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
    if(argc<=2)
    {
        printf("usage:%s ip_address port_number \r\n", basename(argv[0]));
        return 1;
    }
    
    char* ip = argv[1];
    int port = atoi(argv[2]);
    
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET,ip, &address.sin_addr);
    
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd>=0);
    
    struct sockaddr_in client;

    int connfd = connect(sockfd, (struct sockaddr*)&address, sizeof(address));
    if(connfd < 0)
    {
        printf("errno is : %d...connection failed.\r\n", errno );
    }
    else
    {        

        printf("client:----send-----start\r\n");
        char  data1[] = "C2S:hello server..";
        char  data2[] = "C2S:I am client.";
        char  data3[] = "C2S:centos6 system.Bye-Bye";
        send(sockfd, data1, strlen(data1)+1, 0);
        sleep(1);       
        send(sockfd, data2, strlen(data2)+1, 0);
        sleep(1);       
        send(sockfd, data3, strlen(data3)+1, 0);
        sleep(1);
        printf("client:----send-----finish\r\n");
        
        
        
        printf("client:----recv-----start\r\n");
        char buffer[BUFF_SIZE];
        memset(buffer, '\0', sizeof(buffer));
        int ret = recv(sockfd,buffer,sizeof(buffer)-1,0);
        printf("get %d bytes. data:[%s]\r\n", ret, buffer);
        
        
        memset(buffer, '\0', sizeof(buffer));
        ret = recv(sockfd,buffer,sizeof(buffer)-1,0);
        printf("get %d bytes. data:[%s]\r\n", ret, buffer);

        memset(buffer, '\0', sizeof(buffer));
        ret = recv(sockfd,buffer,sizeof(buffer)-1,0);
        printf("get %d bytes. data:[%s]\r\n", ret, buffer);
        printf("client:----recv-----finish\r\n");
        
        //==========================================
        
        
        buffer[BUFF_SIZE];
        memset(buffer, '\0', sizeof(buffer));
        ret = recv(sockfd,buffer,sizeof(buffer)-1,0);
        printf("get %d bytes. data:[%s]\r\n", ret, buffer);
        
        memset(buffer, '\0', sizeof(buffer));
        ret = recv(sockfd,buffer,sizeof(buffer)-1,0);
        printf("get %d bytes. data:[%s]\r\n", ret, buffer);

        memset(buffer, '\0', sizeof(buffer));
        ret = recv(sockfd,buffer,sizeof(buffer)-1,0);
        printf("get %d bytes. data:[%s]\r\n", ret, buffer);

        close(connfd);
    }

    printf("-----connect closed----\r\n");

    close(sockfd);
    
    return 0;
}
