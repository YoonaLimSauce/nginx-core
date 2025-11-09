// 和网络中 连接/连接池 有关的函数

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

// 从连接池中获取一个空闲连接
// 【当一个客户端连接TCP进入，把这个连接和连接池中的一个连接【对象】绑到一起】
PointNgxConnectionType CSocket::NgxGetConnection(int socket_descriptor)
{
    PointNgxConnectionType connection = member_point_free_connections;      // 空闲连接链表头

    if(NULL == connection)
    {
        // 系统会控制连接数量，防止空闲连接被耗尽
        NgxLogStandardError(0, "CSocket::NgxGetConnection()中空闲链表为空,这不应该!");
        return NULL;
    }

    member_point_free_connections = connection->data;       // 指向连接池中下一个未用的节点
    member_free_connection_number--;                        // 空闲连接-1

    // (1)  先把connection指向的对象中有用的东西保存
    uintptr_t instance = connection->instance;                      // 一般connection->instance在刚构造连接池时这里是1【失效】
    uint64_t package_sequence = connection->package_sequence;

    // (2)  把有用的数据保存后，清空
    memset(connection, 0, sizeof(NgxConnectionType));
    connection->fd = socket_descriptor;                     // 保存套接字

    // (3)  之前保存的变量的值赋回来
    connection->instance = !instance;                   // 过期事件标记
    connection->package_sequence = package_sequence;
    connection->package_sequence++;                     // 每次取用都增加1

    return connection;
}

// 归还参数connection所代表的连接到到连接池
void CSocket::NgxFreeConnection(PointNgxConnectionType connection)
{
    connection->data = member_point_free_connections;       // 回收的节点指向原来串起来的空闲链的链头

    connection->package_sequence++;                         // 回收后，值增加1,用于判断某些网络事件是否过期

    member_point_free_connections = connection;             // 修改 原来的链头使链头指向新节点
    member_free_connection_number++;                        // 空闲连接增加1
}