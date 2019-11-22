#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define BUFFER_SIZE  1024

//定义两种http状态码和状态信息
static const char* status_line[2] = {"200 OK", "500 Internal server error"};


//客户端telnet到该服务器上即可获得该文件

int main(int argc, char* argv[])
{   
    if(argc<=3)
    {
        printf("usage:%s ip_address port_number filename.\r\n", basename(argv[0]));
        return 1;
    }
    
    char* ip = argv[1];
    int port = atoi(argv[2]);
    //将目标文件作为程序的第三个参数传入
    const char* file_name = argv[3];
    
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
        //用于保存 http 应答的状态行、头部字段和一个空行的缓存区            
        char header_buf[BUFFER_SIZE];
        memset(header_buf, '\0', sizeof(header_buf));
        
        //用于存放目标文件内容的应用程序缓存
        char* file_buf;
        
        //用于获取目标文件的属性，比如是否为 目录，文件大小等
        struct stat file_stat;
        //记录目标文件是否是有效文件
        bool valid=true;
        //缓存区 header_buf 目前已经使用了多少字节空间
        int len = 0;
        if(stat(file_name, &file_stat) < 0)
        {
            //not exist。目标文件不存在
            valid = false;
        }
        else
        {
            if(S_ISDIR(file_stat.st_mode))
            {
                valid = false;//目标文件是一个目录
            }
			//当前用户有读取目标文件的权限
            else if(file_stat.st_mode & S_IROTH)
            {
				//动态分配缓冲区 fiel_buf，并指定其大小为目标文件的大小 +1，然后将目标文件读入缓冲区file_buf
                int fd = open(file_name, O_RDONLY);
                file_buf = new char[file_stat.st_size + 1];
                memset(file_buf, '\0', file_stat.st_size +1);
                if(read(fd, file_buf, file_stat.st_size)< 0)
                {
                    valid = false;
                }
            }
            else
            {
                valid = false;
            }        
        }
        ////如果目标文件有效，则发送正常的http应答
        if(valid)
        {
			//下面这部分内容将http应答的状态行、"Content-length"头部字段和一个空行依次加入header_buf中
            ret = snprintf(header_buf, BUFFER_SIZE -1, "%s %s \r\n", "HTTP/1.1", status_line[0]);
            len += ret;
            ret = snprintf(header_buf+len, BUFFER_SIZE-1-len, "Content-length:%d\r\n", file_stat.st_size);
            len += ret;
            ret = snprintf(header_buf+len, BUFFER_SIZE-1-len,"%s", "\r\n");
            
            /////利用 writev 将 header_buf和file_buf 的内容一并写出
            struct iovec iv[2];
            iv[0].iov_base = header_buf;
            iv[0].iov_len = strlen(header_buf);
            iv[1].iov_base = file_buf;
            iv[1].iov_len = file_stat.st_size;
            ret = writev(connfd, iv, 2);
        }
        else
        {
			//如果目标文件无效，则通知客户端，服务器发生了内部错误
            ret = snprintf(header_buf, BUFFER_SIZE-1, "%s %s \r\n","HTTP/1.1",status_line[1]);
            len += ret;
            ret = snprintf(header_buf + len, BUFFER_SIZE-1-len, "%s", "\r\n");
            send(connfd,header_buf, strlen(header_buf), 0);
        }
       
     
        close(connfd);
        delete [] file_buf;
    }  

    close(sockfd);
    
    return 0;
}

