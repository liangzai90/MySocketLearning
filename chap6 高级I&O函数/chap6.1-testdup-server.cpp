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
    
    int ret = bind(sockfd,(struct sockaddr*)&address, sizeof(address));
    assert(ret!=-1);
    
    ret = listen(sockfd,5);
    assert(ret!=-1);
    
    struct sockaddr_in client;
    socklen_t client_addlength = sizeof(client);
    int connfd = accept(sockfd, (struct sockaddr*)&client, &client_addlength);
    if(connfd < 0)
    {
        printf("errno is : %d\r\n", errno );
    }
    else
    {               
        printf("server:----recv-----start\r\n");
        char buffer[BUFF_SIZE];
        memset(buffer, '\0', sizeof(buffer));
        int ret = recv(connfd,buffer,sizeof(buffer)-1,0);
        printf("get %d bytes. data:[%s]\r\n", ret, buffer);
        
        memset(buffer, '\0', sizeof(buffer));
        ret = recv(connfd,buffer,sizeof(buffer)-1,0);
        printf("get %d bytes. data:[%s]\r\n", ret, buffer);

        memset(buffer, '\0', sizeof(buffer));
        ret = recv(connfd,buffer,sizeof(buffer)-1,0);
        printf("get %d bytes. data:[%s]\r\n", ret, buffer);
        printf("server:----recv-----finish\r\n");
        sleep(1);


        printf("server:----send-----start\r\n");
        char  data1[] = "S2C:hello client..";
        char  data2[] = "S2C:I am server.";
        char  data3[] = "S2C:centos7 system.Bye-Bye";
        send(connfd, data1, strlen(data1)+1, 0);
        
        sleep(1);       
        send(connfd, data2, strlen(data2)+1, 0);
        sleep(1);       
        send(connfd, data3, strlen(data3)+1, 0);
        sleep(1);
        printf("server:----send-----finish\r\n");
        
        //========================================       

        close(STDOUT_FILENO);        
        dup(connfd);  //N messages , once recv.     
        printf("S2C:==dup==I am server.centos7.");
        printf("S2C:==dup==Hello client...");
        printf("S2C:==dup==bye-bye.");
        
        
        close(connfd);
    }
    
    printf("server----------connection closed--------");

    close(sockfd);
    
    return 0;
}
