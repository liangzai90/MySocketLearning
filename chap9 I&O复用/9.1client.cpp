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
    if(argc<2)
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
       
    if(connect(sock, (struct sockaddr*)&serverSocket, sizeof(serverSocket)) != -1)
    {
        char str1[] = "C2S-Msg1:hello server.\r\n";
        send(sock, str1, strlen(str1), 0);

        char str2[] = "C2S-Msg2:AAAAA2\r\n";
        send(sock, str2, strlen(str2), 0);

        char str3[] = "C2S-Msg3:BBBBB3\r\n";
        send(sock, str3, strlen(str3), 0);

        char str4[] = "C2S-Msg4:DDDDD4\r\n";
        send(sock, str4, strlen(str4), 0);

        char str5[] = "C2S-Msg5:EEEEE5\r\n";
        send(sock, str5, strlen(str5), 0);

        char str6[] = "C2S-Msg6:FFFFFF6\r\n";
        send(sock, str6, strlen(str6), 0);

        char str7[] = "C2S-Msg7:THIS IS OOB MSG.\r\n";
        //send(sock, str7, strlen(str7), MSG_OOB);             

        char str8[] = "C2S-Msg8:GGGGGGGGG8\r\n";
        send(sock, str8, strlen(str8), 0);             

        char str9[] = "C2S-Msg9:HHHHHHHH9\r\n";
        send(sock, str9, strlen(str9), 0);             

        char str10[] = "C2S-Msg10:KKKKKKKKKKKK10\r\n";
        send(sock, str10, strlen(str10), 0);             
    }
    
    close(sock);    

    return 0;
}
