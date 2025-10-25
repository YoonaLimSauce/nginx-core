#ifndef __NGX_C_SOCKET_H__
#define __NGX_C_SOCKET_H__

#include <vector>

#ifndef __cplusplus
#include <stdbool.h>
#endif

// 网络相关宏定义
#define NGX_LISTEN_BACKLOG 511     // 已完成连接队列支持的最大队列数

// 网络结构体定义
typedef struct NgxListeningStruct       // 和监听端口相关的结构
{
    int port;       // 监听的端口号
    int fd;         // 套接字句柄socket
} NgxListeningType, * PointNgxListeningType;

// socket套接字相关的类
class CSocket
{
public:
    CSocket();                                                      // 构造函数
    virtual ~CSocket();                                             // 析构函数

    virtual bool Initialize();                                      // 初始化函数

private:
    bool NgxOpenListeningSockets();                                 // 监听端口【支持多个端口】
    void NgxCloseListeningSockets();                                // 关闭监听套接字
    bool SetNonblocking(int socket_fd);                             // 设置非阻塞套接字

    int member_listen_port_count;                                   // 监听的端口数量
    std::vector<PointNgxListeningType> member_listen_socket_list;   // 监听套接字队列
};

#endif
