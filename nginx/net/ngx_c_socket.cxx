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
    // 配置
    member_listen_port_count = 1;           // 监听一个端口
    member_worker_connections = 1;          // epoll连接最大项数

    // epoll相关
    member_epoll_handle = -1;               // epoll返回的句柄
    member_point_connections = NULL;        // 连接池【连接数组】
    member_point_free_connections = NULL;   // 接池中空闲的连接链
}

// 析构函数
CSocket::~CSocket()
{
    // 释放内存
    // (1)  监听端口相关内存
    std::vector<PointNgxListeningType>::iterator position;
    for(position = member_listen_socket_list.begin(); member_listen_socket_list.end() != position; position++)
    {
        delete (* position);
    }
    member_listen_socket_list.clear();

    // (2)  连接池内存
    if(NULL != member_point_connections)
    {
        delete[] member_point_connections;
    }
}

// 初始化函数【fork()子进程前执行】
bool CSocket::Initialize()
{
    ReadConfiguration();         // 读配置项
    return NgxOpenListeningSockets();       // 打开监听端口
}

// 读配置文件中的网络相关的配置项
void CSocket::ReadConfiguration()
{
    CConfig* point_configuration = CConfig::GetInstance();
    member_worker_connections = point_configuration->GetIntDefault("WorkerConnections", member_worker_connections);     // epoll连接的最大项数
    member_listen_port_count = point_configuration->GetIntDefault("ListenPortCount", member_listen_port_count);     // 需要监听的端口数量
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

// (1)  epoll功能初始化，子进程NgxWorkerProcessInitialize()中进行
int CSocket::NgxEpollInitialize()
{
    // (1)  epoll_create的参数要求, 参数>0
    //      epoll_create的参数为, 指定监听的文件描述符数量上限，早期版本中该参数用于限制红黑树节点数量，但从Linux 2.6.8内核起，该参数已废弃，只需设置为正整数即可。
    // 创建一个epoll对象，创建一个红黑树，创建一个双向链表
    member_epoll_handle = epoll_create(member_worker_connections);      // 直接以epoll连接的最大项数为参数，肯定是>0的

    if(-1 == member_epoll_handle)
    {
        NgxLogStandardError(errno, "CSocket::NgxEpollInitialize()中epoll_create()失败.");
        exit(2);        // 发生致命错误, 由操作系统负责释放资源, 直接退出程序
    }

    // (2)  创建连接池【数组】
    member_connection_number = member_worker_connections;       // 记录当前连接池中连接总数
    // 连接池【数组】
    member_point_connections = new NgxConnectionType[member_connection_number];     // new不可以失败, 如果失败直接报异常, 不处理

    int i = member_connection_number;       // 连接池中连接数
    PointNgxConnectionType next = NULL;
    PointNgxConnectionType connections = member_point_connections;       // 连接池数组首地址

    do
    {
        i--;

        connections[i].data = next;             // 设置连接对象的next指针, 第一次循环时next = NULL
        connections[i].fd = -1;                 // 初始化连接，无socket和连接池中的连接【对象】绑定
        connections[i].instance = 1;            // 失效标志位设置为1【失效】
        connections[i].package_sequence = 0;    // 当前序号统一从0开始

        next = &connections[i];
    } while (i);        // 循环直至数组首地址

    member_point_free_connections = next;       // 设置空闲连接链表头指针
    member_free_connection_number = member_connection_number;       // 空闲连接链表长度

    // (3)  遍历所有监听socket【监听端口】，为每个监听socket增加一个连接池中的连接
    std::vector<PointNgxListeningType>::iterator position;
    for(position = member_listen_socket_list.begin(); position != member_listen_socket_list.end(); position++)
    {
        connections = NgxGetConnection((* position)->fd);     // 从连接池中获取一个空闲连接对象
        
        if(NULL == connections)
        {
            // 致命问题, 初始化的时候连接池不应该为空
            NgxLogStandardError(errno, "CSocket::NgxEpollInitialize()中NgxGetConnection()失败.");
            exit(2);        // 
        }

        connections->listening = (* position);      // 连接对象 和监听对象关联，通过连接对象找监听对象
        (* position)->connection = connections;     // 监听对象 和连接对象关联，通过监听对象找连接对象

        // 对监听端口的读事件设置处理方法
        connections->read_handler = &CSocket::NgxEventAcceptHandler;

        // 往监听socket上增加监听事件，从而开始让监听端口履行其职责
        // 【只有这行，端口能连上，才会触发ngx_epoll_process_events()里边的epoll_wait()往下走】
        if(-1 == NgxEpollAddEvent(  (* position)->fd,           // socekt句柄
                                    1, 0,                       // 读，写事件【参数2：readevent=1, 参数3：writeevent=0】
                                    0,                          // 补充标记
                                    EPOLL_CTL_ADD,              // 事件类型【增加，删除，修改】
                                    connections))               // 连接池中的连接
        {
            exit(2);
        }
    }       // end for(position = member_listen_socket_list.begin(); position != member_listen_socket_list.end(); position++)
    
    return 1;
}

// (2)  监听端口开始工作，为其增加读事件
/*
 *  epoll增加事件
 *  fd:             socket句柄
 *  read_event:     表示是否是读事件, 0是, 1不是
 *  write_event:    表示是否是写事件, 0是, 1不是
 *  other_flag:     其他标记
 *  event_type:     事件类型, 使用枚举值的宏定义, 如增加，删除，修改
 *  connection:     连接池中的连接
 * 返回值：成功返回1，失败返回-1
 */
int CSocket::NgxEpollAddEvent(int fd, int read_event, int write_event, uint32_t other_flag, uint32_t event_type, PointNgxConnectionType connection)
{
    struct epoll_event socket_epoll_event;
    memset(&socket_epoll_event, 0, sizeof(epoll_event));

    if(1 == read_event)
    {
        // 读事件, 官方nginx没有使用EPOLLERR
        socket_epoll_event.events = EPOLLIN | EPOLLRDHUP;       
                // EPOLLIN读事件 = read ready【客户端三次握手连接进来，也属于一种可读事件】                         
                // EPOLLRDHUP 客户端关闭连接
                // EPOLLERR/EPOLLRDHUP 实际上是通过触发读写事件进行读写操作recv write来检测连接异常

        // socket_epoll_event.events |= EPOLLET;
                // 只支持非阻塞socket的高速模式【ET：边缘触发】
                // 设置EPOLLET，则客户端连入时，epoll_wait()只会返回一次该事件
                // EPOLLLT【水平触发：低速模式】
                // 客户端连入时，epoll_wait()会被触发多次，一直到用accept()来处理
    }
    else
    {
        // TODO: nothing
    }

    if(0 != other_flag)
    {
        socket_epoll_event.events |= other_flag;
    }

    // Nginx官方源码
    socket_epoll_event.data.ptr = (void *)((uintptr_t)connection | connection->instance);       // 后续来事件时，用epoll_wait()后，取出标志符

    if(-1 == epoll_ctl(member_epoll_handle, event_type, fd, &socket_epoll_event))
    {
        NgxLogStandardError(errno, "CSocket::NgxEpollAddEvent()中epoll_ctl(%d, %d, %d, %u, %u)失败.", fd, read_event, write_event, other_flag, event_type);
        return -1;
    }

    return 1;
}

// 开始获取发生的事件消息
// 参数 timer：epoll_wait()阻塞的时长，单位是毫秒；
// 返回值，1：正常返回  ,0：有问题返回，无论何返回值都应该保持进程继续运行
int CSocket::NgxEpollProcessEvents(int timer)
{
    // 等待事件，事件会返回到m_events里，最多返回NGX_MAX_EVENTS个事件【因为数字只分配了对应长度的内存】；
    // 阻塞timer, 直到
    //      (a)阻塞时间到达阈值
    //      (b)阻塞期间收到事件会立刻返回
    //      (c)调用时有事件也会立刻返回
    //      (d)中断信号

    // 如果timer为-1则一直阻塞，如果timer为0则立即返回，即便没有任何事件
    // 返回值： 有错误发生返回-1，错误在errno中
    //          超时，则返回0
    //          如果返回值>0则表示成功捕获到[返回值]个事件
    int events = epoll_wait(member_epoll_handle, member_events, NGX_MAX_EVENTS, timer);

    if(-1 == events)
    {
        // EINTR错误的产生：当阻塞于系统调用的一个进程捕获信号且相应信号处理函数返回时，系统调用可能返回一个EINTR错误
        if(EINTR == errno)
        {
            // 信号所致，直接返回，一般认为这不是毛病，打印日志记录，因为一般也不会人为给worker进程发送消息
            NgxLogErrorCore(NGX_LOG_INFORMATION, errno, "CSocket::NgxEpollProcessEvents()中epoll_wait()失败!");
            return 1;       // 正常返回
        }
        else
        {
            // 有程序问题, 记录日志
            NgxLogErrorCore(NGX_LOG_ALERT, errno, "CSocket::NgxEpollProcessEvents()中epoll_wait()失败!");
            return 0;       // 非正常返回
        }
    }       // end if(-1 == events)

    if(0 == events)     // 超时，但没事件来
    {
        if(-1 != timer)
        {
            // 要求epoll_wait阻塞一定的时间而不是一直阻塞，这属于阻塞到时间了，则正常返回
            return 1;
        }
        // 无限等待【所以不存在超时】，但却没返回任何事件，这应该不正常有问题
        NgxLogErrorCore(NGX_LOG_ALERT, 0, "CSocket::NgxEpollProcessEvents()中epoll_wait()没有超时却没有返回任何事件!");
        return 0;       // 非正常返回 
    }

    // 收到事件
    PointNgxConnectionType      connection;
    uintptr_t                   instance;
    uint32_t                    read_events;

    for(int i = 0; i < events; i++)     // 遍历本次epoll_wait返回的所有事件，events是返回的实际事件数量
    {
        connection = (PointNgxConnectionType)(member_events[i].data.ptr);                   // ngx_epoll_add_event()给进去的，这里取出来
        instance = (uintptr_t)connection & 1;                                               // 将地址的最后一位取出来，用instance变量标识
        connection = (PointNgxConnectionType)((uintptr_t)connection & (uintptr_t) ~1);      // 最后1位干掉，得到真正的c地址

        // nginx源码的判断分支
        if(-1 == connection->fd)        // 套接字关联一个连接池中的连接【对象】时，套接字值对应connection->fd
        {
            // 用epoll_wait取得三个事件
            // 处理第一个事件时，因为业务需要，我们把这个连接关闭，应该把connection->fd设置为-1
            // 第二个事件照常处理
            // 第三个事件，如果跟第一个事件对应的是同一个连接，那这个条件就会成立；否则属于过期事件，不该处理

            NgxLogErrorCore(NGX_LOG_DEBUG, 0, "CSocket::NgxEpollProcessEvents()中遇到了fd=-1的过期事件:%p.", connection);
            continue;       // 这种事件就不处理即可
        }

        if(connection->instance != instance)
        {
            // 过期事件
            NgxLogErrorCore(NGX_LOG_DEBUG, 0, "CSocket::NgxEpollProcessEvents()中遇到了instance值改变的过期事件:%p.", connection);
            continue;       // 这种事件就不处理即可
        }

        // 正常事件
        read_events = member_events[i].events;      // 取出事件类型
        if(read_events & (EPOLLERR | EPOLLHUP))        // 发生了错误或者客户端断连
        {
            // 加上读写标记，方便后续代码处理
            read_events |= EPOLLIN | EPOLLOUT;      // EPOLLIN：表示对应的链接上有数据可以读出
                                                    // EPOLLOUT：表示对应的连接上可以写入数据发送【写准备好】
        }

        if(read_events & EPOLLIN)      // 读事件
        {
            // 客户端新连入
            (this->* (connection->read_handler))(connection);       
                    // 如果新连接进入，这里执行的应该是CSocekt::NgxEventAcceptHandler
                    // 如果是已经连入，发送数据到这里，则这里执行的应该是CSocekt::NgxWaitRequestHandler
        }

        if(read_events & EPOLLOUT)      // 写事件
        {
            // TODO: nothing
            NgxLogStandardError(errno, "写事件发生, 待后续处理");
        }
    }       // end for(int i = 0; i < events; i++)

    return 1;
}