#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>     // uintptr_t
#include <stdarg.h>     // va_args
#include <unistd.h>     // STDERR_FILENO
#include <time.h>       // localtime_r
#include <fcntl.h>      // open
#include <errno.h>      // errno
#include <sys/time.h>   // gettimeofday
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include "ngx_c_conf.h"
#include "ngx_macro.h"
#include "ngx_global.h"
#include "ngx_func.h"
#include "ngx_c_socket.h"

// 构造函数
CSocket::CSocket()
{
    member_listen_port_count = 1;      // 监听一个端口
}

// 析构函数
CSocket::~CSocket()
{
    std::vector<PointNgxListeningType>::iterator position;
    for(position = member_listen_socket_list.begin(); member_listen_socket_list.end() != position; position++)
    {
        delete (* position);
    }
    member_listen_socket_list.clear();
}

// 初始化函数【fork()子进程前执行】
bool CSocket::Initialize()
{
    return NgxOpenListeningSockets();
}

// 监听端口【支持多端口监听】
// 在创建worker进程之前执行
bool CSocket::NgxOpenListeningSockets()
{
    CConfig* point_config = CConfig::GetInstance();
    member_listen_port_count = point_config->GetIntDefault("ListenPortCount", member_listen_port_count);        // 读取配置文件中监听的端口数量

    int                     int_socket;                 // 套接字socket
    int                     int_port;                   // 端口port
    char                    string_information[100];    // 字符串信息
    struct sockaddr_in      server_address;             // 服务器的地址结构体

    // 初始化
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;                // TCP协议为IPV4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // 监听本地所有的IP地址

    for(int i = 0; i < member_listen_port_count; i++)
    {
        // 参数1：AF_INET：使用ipv4协议
        // 参数2：SOCK_STREAM：使用TCP，表示可靠连接
        // 参数3：0
        int_socket = socket(AF_INET, SOCK_STREAM, 0);       // 系统函数，成功返回非负描述符，出错返回-1
        if(-1 == int_socket)
        {
            NgxLogStandardError(errno, "CSocket::Initialize()中socket()失败, i=%d.", i);
            // 如果以往有成功创建的socket, 需要先释放成功创建的socket, 然后在退出程序
            return false;
        }

        // setsockopt（）:设置套接字参数选项；
        // 参数2：是表示级别，和参数3配套使用
        // 参数3：允许重用本地地址
        // 设置 SO_REUSEADDR，解决TIME_WAIT这个状态导致bind()失败的问题
        int re_use_address = 1;     // 1: 允许地址复用
        if(-1 == setsockopt(int_socket, SOL_SOCKET, SO_REUSEADDR, (const void*) &re_use_address, sizeof(re_use_address)))
        {
            NgxLogStandardError(errno, "CSocket::Initialize()中setsockopt(SO_REUSEADDR)失败, i=%d.", i);
            close(int_socket);      // 无需处理是否正常执行关闭套接字
            return false;
        }

        // 设置非阻塞套接字socket
        if(false == SetNonblocking(int_socket))
        {
            NgxLogStandardError(errno, "CSocket::Initialize()中SetNonblocking()失败, i=%d.", i);
            close(int_socket);
            return false;
        }

        // 设置服务器监听的地址和端口
        string_information[0] = 0;
        sprintf(string_information, "ListenPort%d", i);
        int_port = point_config->GetIntDefault(string_information, 65535);
        server_address.sin_port = htons((in_port_t) int_port);

        // 绑定服务器地址结构体
        if(-1 == bind(int_socket, (struct sockaddr*) &server_address, sizeof(server_address)))
        {
            NgxLogStandardError(errno, "CSocket::Initialize()中bind()失败, i=%d.", i);
            close(int_socket);
            return false;
        }

        // 监听服务器端口
        if(-1 == listen(int_socket, NGX_LISTEN_BACKLOG))
        {
            NgxLogStandardError(errno, "CSocket::Initialize()中listen()失败, i=%d.", i);
            close(int_socket);
            return false;
        }

        PointNgxListeningType point_listen_socket_item = new NgxListeningType;
        memset(point_listen_socket_item, 0, sizeof(point_listen_socket_item));
        point_listen_socket_item->port = int_port;                              // 端口号
        point_listen_socket_item->fd = int_socket;                              // 套接字句柄
        NgxLogErrorCore(NGX_LOG_INFORMATION, 0, "监听%d端口成功!", int_port);    // 记录日志
        member_listen_socket_list.push_back(point_listen_socket_item);          // 监听端口队列
    }   // end for(int i = 0; i < member_listen_port_count; i++)
    return true;
}

// 设置socket连接为非阻塞模式
bool CSocket::SetNonblocking(int socket_fd)
{
    int not_blocking = 1;       // 0：清除，1：设置 
    if(-1 == ioctl(socket_fd, FIONBIO, &not_blocking))      // FIONBIO：设置/清除非阻塞I/O标记：0：清除，1：设置
    {
        NgxLogStandardError(errno,"CSocekt::setnonblocking()中ioctl(FIONBIO)失败.");
        return false;
    }
    return true;

    /* 
    // fcntl:file control【文件控制】相关函数，执行各种描述符控制操作
    // 参数1：所要设置的描述符，这里是套接字【也是描述符的一种】
    int opts = fcntl(sockfd, F_GETFL);  //用F_GETFL先获取描述符的一些标志信息
    if(opts < 0) 
    {
        NgxLogStandarError(errno,"CSocekt::setnonblocking()中fcntl(F_GETFL)失败.");
        return false;
    }
    opts |= O_NONBLOCK; //把非阻塞标记加到原来的标记上，标记这是个非阻塞套接字【如何关闭非阻塞呢？opts &= ~O_NONBLOCK,然后再F_SETFL一下即可】
    if(fcntl(sockfd, F_SETFL, opts) < 0) 
    {
        ngx_log_stderr(errno,"CSocekt::setnonblocking()中fcntl(F_SETFL)失败.");
        return false;
    }
    return true;
    */
}

// 关闭socket
void CSocket::NgxCloseListeningSockets()
{
    for(int i = 0; i < member_listen_port_count; i++)       // 关闭多个监听端口
    {
        // NgxLogStandardError(0, "端口是%d, socket_id是%d.", member_listen_socket_list[i]->port, member_listen_socket_list->fd);
        close(member_listen_socket_list[i]->fd);
        NgxLogErrorCore(NGX_LOG_INFORMATION, 0, "关闭监听端口%d!", member_listen_socket_list[i]->port);     // 记录日志
    }
}