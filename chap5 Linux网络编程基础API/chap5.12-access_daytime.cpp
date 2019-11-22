#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>

int main(int argc, char* argv[])
{
    assert(argc ==2);
    char* host = argv[1];
    //get dest host ip info
    struct hostent* hostinfo = gethostbyname(host);
    assert(hostinfo);
    
    //get daytime info
    struct servent* servinfo = getservbyname("daytime", "tcp");
    assert(servinfo);

//// attention the struct convert
    struct in_addr pNetIp = *(struct in_addr*)*hostinfo->h_addr_list;  
    char serverHostIp[100] ;
    inet_ntop(AF_INET, &pNetIp, serverHostIp, 100);
        
    printf("server host ip:[%s].daytime port is %d.\r\n",serverHostIp, ntohs(servinfo->s_port));
    
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = servinfo->s_port;
    
    // Attention! directly use servent 
    address.sin_addr = *(struct in_addr*)*hostinfo->h_addr_list;
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int result = connect(sockfd, (struct sockaddr*)&address, sizeof(address));
    assert(result != -1);
    
    char buffer[128];
    result = read(sockfd, buffer, sizeof(buffer));
    assert(result > 0);
    buffer[result] = '\0';
    printf("the day time is: %s", buffer);

    close(sockfd);
    return 0;
}
