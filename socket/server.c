#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_PORT 9000        // 服务器要监听的端口号
                                // 一般1024以下的端口很多都是属于周知端口

int main(int argc, char* argv[], char* argp[])
{
    // 服务器的socket套接字【文件描述符】
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);        // 创建服务器的socket

    struct sockaddr_in server_address;          // 服务器的地址结构体
    memset(&server_address, 0, sizeof(server_address));

    // 设置本服务器要监听的地址和端口
    server_address.sin_family = AF_INET;                        // 选择协议族为IPV4
    server_address.sin_port = htons(SERVER_PORT);               // 绑定自定义的端口号
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);         // 监听本地所有的IP地址；INADDR_ANY表示的是一个服务器上所有的网卡（服务器可能不止一个网卡）多个本地ip地址都进行绑定端口号，进行侦听

    // setsockopt（）:  设置一些套接字参数选项
    // parameter 2:     表示级别，和参数3配套使用，参数3如果确定，参数2就确定
    // parameter 3:     允许重用本地地址
    int reuse_address = 1;      // 开启
    if(-1 == setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void*) &reuse_address, sizeof(reuse_address)))
    {
        char* point_error_information = strerror(errno);
        printf("setsocketopt(SO_REUSEADDR)返回值为%d, 错误码为%d, 错误信息为:%s;\n", -1, errno, point_error_information);
        return -1;
    }

    int result = -1;
    result = bind(listen_fd, (struct sockaddr*) &server_address, sizeof(server_address));        // 绑定服务器地址结构体
    if(-1 == result)
    {
        char* point_error_information = strerror(errno);
        printf("bind返回的值为%d, 错误码为:%d, 错误信息为:%s;\n", result, errno, point_error_information);
        return -1;
    }

    result = listen(listen_fd, 32);          // 参数2表示服务器可以积压的未处理完的连入请求总个数
    if(-1 == result)
    {
        char* point_error_information = strerror(errno);
        printf("listen返回的值为%d, 错误码为:%d, 错误信息为:%s;\n", result, errno, point_error_information);
        return -1;
    }

    int connect_fd;
    char receive_content[1000 + 1];
    const char* point_content = "I sent something to client!\n";        // 

    for(;;)
    {
        // 阻塞在这里，等待Client客户端连接
        // 【注意这里返回的是一个新的socket——connfd，后续本服务器就用connfd和客户端之间收发数据，而原有的lisenfd依旧用于继续监听其他连接】
        connect_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL);

        // 读取客户端发送的数据包
        read(connect_fd, receive_content, 1000);

        // 发送数据包给客户端
        write(connect_fd, point_content, strlen(point_content));        // 注意第一个参数是accept返回的套接字

        // 直接关闭套接字连接
        close(connect_fd);
    }

    close(listen_fd);       // 一般走不到这里

    return 0;
}