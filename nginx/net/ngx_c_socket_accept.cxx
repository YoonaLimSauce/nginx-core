// 和网络中接受连接【accept】有关的函数

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

// 建立新连接函数
void CSocket::NgxEventAcceptHandler(PointNgxConnectionType old_connection)
{
    // listen套接字上用的不是ET【边缘触发】，而是LT【水平触发】
    // 客户端连入如果不处理，本函数会被多次调用
    // 可以不必多次accept()，只执行一次accept()
    // 可以避免本函数被执行太久

    // 本函数应该尽快返回，以免阻塞程序运行
    struct sockaddr             my_socket_address;      // 远端服务器的socket地址
    socklen_t                   socket_length;
    int                         error_number;
    int                         error_level;
    int                         socket_accept_handler;
    static int                  use_accept4 = 1;        // 使用accept4()函数
    PointNgxConnectionType      new_connection;         // 代表连接池中的一个连接

    socket_length = sizeof(my_socket_address);
    
    for(;;)
    {
        if(use_accept4)
        {
            socket_accept_handler = accept4(old_connection->fd, &my_socket_address, &socket_length, SOCK_NONBLOCK);      // 从内核获取一个用户端连接，最后一个参数SOCK_NONBLOCK表示返回一个非阻塞的socket，节省一次ioctl【设置为非阻塞】调用
        }
        else
        {
            socket_accept_handler = accept(old_connection->fd, &my_socket_address, &socket_length);
        }

        if(-1 == socket_accept_handler)
        {
            error_number = errno;

            // 对accept、send和recv而言，事件未发生时errno通常被设置成EAGAIN或者EWOULDBLOCK
            if(EAGAIN == error_number)
            {
                // 一般只有用一个循环不断的accept()取走所有的连接，才会有这个错误
                return;
            }

            error_level = NGX_LOG_ALERT;

            if(ECONNABORTED == error_number)       // ECONNRESET错误发生在对方意外关闭套接字后【客户端中的软件放弃了一个已建立的连接--由于超时或者其它失败而中止接连】
            {
                // 该错误被描述为"软件引起的连接中止"
                // 原因在于当服务和客户进程在完成用于TCP连接的“三次握手”后，客户TCP却发送了一个RST（复位），在服务进程看来，该连接已排队，等服务进程调用accept的时候RST却到达了
                // 服务器进程一般可以忽略该错误，直接再次调用accept
                error_level = NGX_LOG_ERROR;
            }
            else if(EMFILE == error_number || ENFILE == error_number)
                // EMFILE:进程的fd已用尽【已达到系统所允许单一进程所能打开的文件/套接字总数】
                // ENFILE这个errno的存在，表明一定存在system-wide的resource limits，而不仅仅有process-specific的resource limits
            {
                error_level = NGX_LOG_CRITICAL;
            }

            if(use_accept4)
            {
                NgxLogErrorCore(error_level, errno, "CSocket::NgxEventAcceptHandler()中accept4()失败!");
            }
            else
            {
                NgxLogErrorCore(error_level, errno, "CSocket::NgxEventAcceptHandler()中accept()失败!");
            }

            if(use_accept4 && error_number == ENOSYS)       // accept4()函数没实现
            {
                use_accept4 = 0;        // 标记不使用accept4()函数，改用accept()函数
                continue;
            }

            if(ECONNABORTED == error_number)        // 客户端关闭套接字
            {
                // TODO: nothing
            }

            if(error_number == EMFILE || error_number == ENFILE)
            {
                // TODO: nothing
            }
            return;
        }       // end if(-1 == socket_accept_handler)

        // 表示accept4() | accept()成功
        new_connection = NgxGetConnection(socket_accept_handler);

        if(NULL == new_connection)
        {
            // 连接池中无空闲连接，需要直接关闭socekt并返回
            if(-1 == close(socket_accept_handler))
            {
                NgxLogErrorCore(NGX_LOG_ALERT, errno, "CSocket::NgxEventAcceptHandler()中close(%d)失败!", socket_accept_handler);
            }
            return;
        }

        // TODO: 判断是否连接超过最大允许连接数

        // 成功的获取连接池中的一个连接
        memcpy(&new_connection->source_socket_address, &my_socket_address, socket_length);      // 拷贝客户端地址到连接对象

        if(!use_accept4)
        {
            // 用accept()取得的socket，要设置为非阻塞
            if(false == SetNonblocking(socket_accept_handler))
            {
                // 设置非阻塞失败
                NgxCloseAcceptedConnection(new_connection);
                return;
            }
        }

        new_connection->listening = old_connection->listening;                  // 连接对象和监听对象关联
        new_connection->write_ready = 1;                                        // 标记可以写，新连接写事件肯定是ready
        new_connection->read_handler = &CSocket::NgxWaitRequestHandler;         // 设置数据来时的读处理函数

        // 客户端应该主动发送第一次的数据，将读事件加入epoll监控
        if(-1 == NgxEpollAddEvent(socket_accept_handler,        // socket句柄
                                    1, 0,                       // 读，写
                                    EPOLLET,                    // 补充标记【EPOLLET(高速模式，边缘触发ET)】
                                    EPOLL_CTL_ADD,              // 事件类型【增加，删除/修改】
                                    new_connection))            // 连接池中的连接
        {
            // 增加事件失败
            NgxCloseAcceptedConnection(new_connection);
            return;
        }

        break;
    }
}

// accept4() | accept() 得到的socket在处理中产生失败，则关闭连接前需要释放资源
void CSocket::NgxCloseAcceptedConnection(PointNgxConnectionType connection)
{
    int fd = connection->fd;
    NgxFreeConnection(connection);
    connection->fd = -1;

    if(-1 == close(fd))
    {
        NgxLogErrorCore(NGX_LOG_ALERT,errno,"CSocket::NgxCloseAcceptedConnection()中close(%d)失败!",fd);
    }
}