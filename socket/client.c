#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_PORT 9000        // 要连接到的服务器端口，服务器必须在这个端口上listen

int main(int argc, char* argv[], char* argp[])
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);    // 创建客户端的socket

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));

    // 设置要连接到的服务器的信息
    server_address.sin_family = AF_INET;                // 选择协议族为IPV4
    server_address.sin_port = htons(SERVER_PORT);       // 连接到的服务器端口，服务器监听SERVER_PORT

    // 要连接的服务器地址固定写
    if(0 >= inet_pton(AF_INET, "172.20.68.195", &server_address.sin_addr))      // IP地址转换函数,把第二个参数对应的ip地址转换第三个参数
    {
        printf("调用inet_pton()失败, 退出! \n");
        exit(1);
    }

    if(0 > connect(socket_fd, (struct sockaddr*) &server_address, sizeof(server_address)))
    {
        printf("调用connect()失败, 退出! \n");
        exit(1);
    }

    int number;
    char receive_content[1000 + 1];

    while(0 < (number = read(socket_fd, receive_content, 1000)))
    {
        receive_content[number] = 0;
        printf("收到的内容为: %s \n", receive_content);
    }

    close(socket_fd);       // 关闭套接字

    printf("程序执行完毕, 退出! \n");

    return 0;
}