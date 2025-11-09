#ifndef __NGX_C_SOCKET_H__
#define __NGX_C_SOCKET_H__

#include <sys/epoll.h>
#include <sys/socket.h>

#include <vector>

#ifndef __cplusplus
#include <stdbool.h>
#endif

// 网络相关宏定义
#define NGX_LISTEN_BACKLOG  511     // 已完成连接队列支持的最大队列数
#define NGX_MAX_EVENTS      512     // epoll_wait一次能够接收的事件数量上限

typedef struct  NgxListeningStructure   * PointNgxListeningType;
typedef struct  NgxConnectStructure     * PointNgxConnectionType;
typedef class   CSocket                 CSocket;
typedef void (CSocket::* NgxEventHandlerPointType)(PointNgxConnectionType point_connection_pool);        // socket套接字连接池的成员函数指针

// 网络结构体定义
typedef struct NgxListeningStructure            // 和监听端口相关的结构
{
    int                     port;               // 监听的端口号
    int                     fd;                 // 套接字句柄socket
    PointNgxConnectionType   connection;         // 连接池中的一个连接, 指针类型
} NgxListeningType, * PointNgxListeningType;

// TCP连接实例的结构
// 结构表示一个TCP连接【客户端主动发起的、Nginx服务器被动接受的TCP连接】
typedef struct NgxConnectStructure
{
    int                         fd;                         // 套接字句柄socket
    PointNgxListeningType       listening;                  // 监听套接字

    unsigned                    instance:1;                 // 【位域】失效标志位：0：有效，1：失效
    uint64_t                    package_sequence;           // 引入的一个序号，每次分配出去时+1，用于检测错包废包
    struct sockaddr             source_socket_address;      // TCP连接的socket套接字

    // read读TCP网络包相关
    uint8_t                     write_ready;                // 写标记

    NgxEventHandlerPointType    write_handler;              // 读事件的相关处理方法
    NgxEventHandlerPointType    read_handler;               // 写事件的相关处理方法

    PointNgxConnectionType      data;                       // 指针【等价于传统链表里的next成员：后继指针】
} NgxConnectionType, * PointNgxConnectionType;

// socket套接字相关的类
class CSocket
{
public:
    CSocket();                                                              // 构造函数
    virtual ~CSocket();                                                     // 析构函数

    virtual bool Initialize();                                              // 初始化函数

    int NgxEpollInitialize();                                               // epoll功能初始化
    int NgxEpollAddEvent(int fd, int read_event, int write_event, uint32_t other_flag, uint32_t event_type, PointNgxConnectionType connection);                         // epoll增加事件
    int NgxEpollProcessEvents(int timer);                                   // epoll等待接收和处理事件

private:
    bool NgxOpenListeningSockets();                                         // 监听端口【支持多个端口】
    void NgxCloseListeningSockets();                                        // 关闭监听套接字
    bool SetNonblocking(int socket_fd);                                     // 设置非阻塞套接字

    void ReadConfiguration();                                               // 读配置文件

    // 业务处理函数handler
    void NgxEventAcceptHandler(PointNgxConnectionType old_connection);      // 建立新连接
    void NgxWaitRequestHandler(PointNgxConnectionType connection);          // 设置数据来时的读处理函数

    void NgxCloseAcceptedConnection(PointNgxConnectionType connection);     // 用户连入，我们accept()时，得到的socket在处理中产生失败，则资源用这个函数释放

    // 获取对端信息
    size_t NgxSocketNetworkToPresentation(struct sockaddr* socket_address, int port, u_char* text, size_t length);                                                                // 根据参数socket_address，获取地址端口字符串，返回这个字符串的长度

    // 连接池
    PointNgxConnectionType NgxGetConnection(int socket_descriptor);         // 从连接池中获取一个空闲连接
    void NgxFreeConnection(PointNgxConnectionType connection);              // 参数connection所代表的连接插入到连接池中

    int member_listen_port_count;                                           // 监听的端口数量
    std::vector<PointNgxListeningType> member_listen_socket_list;           // 监听套接字队列

    int member_worker_connections;                                          // epoll连接的最大项数
    int member_epoll_handle;                                                // epoll_create返回的句柄

    // 连接池
    PointNgxConnectionType member_point_connections;                        // 连接池的首地址
    PointNgxConnectionType member_point_free_connections;                   // 空闲连接链表头

    int member_connection_number;                                           // 当前进程中所有连接对象的总数【连接池大小】
    int member_free_connection_number;                                      // 连接池中可用连接总数

    struct epoll_event member_events[NGX_MAX_EVENTS];                       // 用于在epoll_wait()中记录返回的所发生的事件
};

#endif
