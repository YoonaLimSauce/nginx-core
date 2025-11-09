// 和网络中 获取一些ip地址等信息 有关的函数

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include "ngx_c_conf.h"
#include "ngx_macro.h"
#include "ngx_global.h"
#include "ngx_func.h"
#include "ngx_c_socket.h"

// 将socket绑定的地址转换为文本格式【根据参数1给定的信息，获取地址端口字符串，返回这个字符串的长度】
// 参数socket_address:      客户端的ip地址信息
// 参数port:                为1，则表示要把端口信息也放到组合成的字符串里，为0，则不包含端口信息
// 参数text:                文本
// 参数length:              文本长度
size_t NgxSocketNetworkToPresentation(struct sockaddr* socket_address, int port, u_char* text, size_t length)
{
    struct sockaddr_in* socket_address_in;
    u_char*             ip_address;

    switch (socket_address->sa_family)
    {
    case AF_INET:
        socket_address_in = (struct sockaddr_in*) socket_address;
        ip_address = (u_char*) &socket_address_in->sin_addr;

        if(port)        // 端口信息也组合到字符串里
        {
            ip_address = NgxStringLengthPrintf(text, length, "%ud.%ud.%ud.%ud:%d",ip_address[0], ip_address[1], ip_address[2], ip_address[3], ntohs(socket_address_in->sin_port));     // 返回的是新的可写地址
        }
        else            // 不需要组合端口信息到字符串中
        {
            ip_address = NgxStringLengthPrintf(text, length, "%ud.%ud.%ud.%ud",ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
        }
        return (ip_address - text);
    
    default:
        return 0;
    }       // end switch (socket_address->sa_family)
}