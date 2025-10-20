#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "ngx_func.h"
#include "ngx_macro.h"
#include "ngx_c_conf.h"

// 描述：守护进程初始化
// 执行失败：返回-1，   
// 执行成功：子进程：返回0; 父进程：返回1
int NgxDaemon()
{
    // (1)  创建守护进程首先，fork()一个子进程出来
    switch(fork())      // fork()出来这个子进程是master进程
    {
        case -1:
            // 创建子进程失败
            NgxLogErrorCore(NGX_LOG_EMERGENCY, errno, "NgxDaemon()中fork()失败!");
            return -1;

        case 0:
            // 子进程
            break;

        default:
            // 父进程, 以往直接退出exit(0); 现在希望回到主流程去释放一些资源
            return 1;
    }   // end switch(fork()) 

    // fork()子进程执行到此
    ngx_parent = ngx_pid;       // ngx_pid是原来父进程的id，子进程的ngx_parent设置为原来父进程的pid
    ngx_pid = getpid();         // 当前子进程的pid要重新取得

    // (2)  脱离终端，终端关闭，将跟此子进程无关
    if(-1 == setsid())
    {
        NgxLogErrorCore(NGX_LOG_EMERGENCY, errno, "NgxDaemon()中setsid()失败!");
        return -1;
    }

    // (3)  设置为0，守护进程会创建文件, 不要被限制文件权限
    umask(0);

    // (4)  以读写方式打开黑洞设备
    int fd = open("/dev/null", O_RDWR);
    if(-1 == fd)
    {
        NgxLogErrorCore(NGX_LOG_EMERGENCY, errno, "NgxDaemon()中open(\"/dev/null\")失败!");
        return -1;
    }

    if(-1 == dup2(fd, STDIN_FILENO))        // 先关闭STDIN_FILENO, 类似于指针指向null，让/dev/null成为标准输入
    {
        NgxLogErrorCore(NGX_LOG_EMERGENCY, errno, "NgxDaemon()中dup2(STDIN)失败!");
        return -1;
    }

    if(-1 == dup2(fd, STDOUT_FILENO))        // 再关闭STDOUT_FILENO，类似于指针指向null，让/dev/null成为标准输出
    {
        NgxLogErrorCore(NGX_LOG_EMERGENCY, errno, "NgxDaemon()中dup2(STDOUT)失败!");
        return -1;
    }

    if(fd > STDERR_FILENO)      // fd应该是3
    {
        if(-1 == close(fd))
        {
            NgxLogErrorCore(NGX_LOG_EMERGENCY, errno, "NgxDaemon()中close(fd)失败!");
            return -1;
        }
    }

    return 0;       // 子进程成功返回0
}