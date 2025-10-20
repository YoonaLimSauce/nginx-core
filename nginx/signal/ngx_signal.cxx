#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#include "ngx_global.h"
#include "ngx_macro.h"
#include "ngx_func.h"

// 信号有关的结构体
typedef struct
{
    int Signal_Number;      // 信号对应的数字编号，每个信号都有对应的#define
    const char* Signal_Name;        // 信号对应的中文名字
    // 信号处理函数, Coder自定义函数, 但是它的参数和返回值是固定的【操作系统要求】
    void (*Handler) (int signal_number, siginfo_t* signal_information, void* user_context);     // 函数指针
} ngx_signal_type;

// 声明信号处理函数
static void NgxSignalHandler(int signo, siginfo_t *siginfo, void *ucontext); //static表示该函数只在当前文件内可见
static void NgxProcessGetStatus(void);       // 获取子进程的结束状态，防止单独kill子进程时子进程变成僵尸进程

// 定义本系统处理的各种信号
ngx_signal_type ngx_signals[] = {
    // signo        signame         handler
    { SIGHUP,       "SIGHUP",       NgxSignalHandler },     // 终端断开信号，对于守护进程常用于reload重载配置文件通知
    { SIGINT,       "SIGINT",       NgxSignalHandler }, 
    { SIGTERM,      "SIGTERM",      NgxSignalHandler },
    { SIGCHLD,      "SIGCHLD",      NgxSignalHandler },     // 子进程退出时，父进程会收到这个信号
    { SIGQUIT,      "SIGQUIT",      NgxSignalHandler },
    { SIGIO,        "SIGIO",        NgxSignalHandler },     // 指示一个异步I/O事件【通用异步I/O信号】
    { SIGSYS,       "SIGSYS",       NULL },     // 忽略这个信号，SIGSYS表示收到了一个无效系统调用，如果我们不忽略，进程会被操作系统杀死
    { 0,            NULL,           NULL }      // 信号对应的数字至少是1，所以可以用0作为一个特殊标记
};

void NgxSignalHandler(int signal_number, siginfo_t* signal_information, void* user_context)
{
    ngx_signal_type*    signal;
    char*               action;     // 字符串，用于记录一个动作, 以往日志文件中写

    // 找到参数signal_number对应信号
    for(signal = ngx_signals; signal->Signal_Number != 0; signal++)
    {
        if(signal_number == signal->Signal_Number)
        {
            break;
        }
    }

    action = (char*) "";    // 当前为空动作

    if(NGX_PROCESS_MASTER == ngx_process)       // master进程，管理进程, 信号驱动的进程
    {
        switch (signal_number)
        {
            case SIGCHLD:       // 子进程退出信号
                ngx_reap = 1;       // //标记子进程状态变化，日后master主进程的for(;;)循环中用【比如重新产生一个子进程】
                break;
            
            default:
                break;
        }   // end switch (signal_number)
    }
    else if(NGX_PROCESS_WORKER == ngx_process)
    {
        // worker进程的往这里走
        // TODO: to be added when necessary
    }
    else
    {
        // 非master非worker进程
        // TODO: nothing to do temporarily
    }   // end if(NGX_PROCESS_MASTER == ngx_process)

    // 日志记录
    if(signal_information && signal_information->si_pid)        // si_pid = sending process ID【发送该信号的进程id】
    {
        NgxLogErrorCore(NGX_LOG_NOTICE, 0, "signal %d (%s) received from %P%s", signal_number, signal->Signal_Name, signal_information->si_pid, action);
    }
    else
    {
        NgxLogErrorCore(NGX_LOG_NOTICE, 0, "signal %d (%s) received %s", signal_number, signal->Signal_Name, action);       // 没有发送该信号的进程id
    }

    // 子进程状态有变化，通常是意外退出
    if(SIGCHLD == signal_number)
    {
        NgxProcessGetStatus();      // 获取子进程的结束状态
    }
}

int NgxInitializeSignals()
{
    ngx_signal_type* signal;       // 指向自定义结构数组的指针 
    struct sigaction signal_action;     // sigaction：系统定义的跟信号有关的一个结构\

    for(signal = ngx_signals; 0 != signal->Signal_Number; signal++)        // 将signo ==0作为一个标记
    {
        memset(&signal_action, 0, sizeof(struct sigaction));

        if(signal->Handler)
        {
            signal_action.sa_sigaction = signal->Handler;       // sa_sigaction：指定信号处理程序(函数)，注意sa_sigaction也是函数指针，是这个系统定义的结构sigaction中的一个成员（函数指针成员）
            signal_action.sa_flags = SA_SIGINFO;        // sa_flags：int型，指定信号的一些选项，设置了该标记(SA_SIGINFO)，就表示信号附带的参数可以被传递到信号处理函数中
        }
        else
        {
            signal_action.sa_handler = SIG_IGN;     // sa_handler:这个标记SIG_IGN给到sa_handler成员，表示忽略信号的处理程序，否则操作系统的缺省信号处理程序很可能把这个进程杀掉
        }   // end if(signal->Handler)

        sigemptyset(&signal_action.sa_mask);        // .sa_mask是个信号集（描述信号的集合），用于表示要阻塞的信号
                                                    // sigemptyset()这个函数咱们在第三章第五节讲过：把信号集中的所有信号清0，本意就是不准备阻塞任何信号
        if(-1 == sigaction(signal->Signal_Number, &signal_action, NULL))
                // 参数1：要操作的信号
                // 参数2：信号处理函数以及执行信号处理函数时候要屏蔽的信号等等内容
                // 参数3：返回以往的对信号的处理方式【跟sigprocmask()函数边的第三个参数是一样的】
        {
            NgxLogErrorCore(NGX_LOG_EMERGENCY, errno, "sigaction(%s) failed!", signal->Signal_Name);        // 显示到日志文件中
            return -1;
        }
        else
        {
            NgxLogStandardError(0, "sigaction(%s), succeed!", signal->Signal_Name);     // 屏幕上打印, release版本删除
        }
    }   // end for(signal = ngx_signals; 0 != signal->Signal_Number; sig++) 
    return 0;   // 成功
}

// 获取子进程的结束状态，防止单独kill子进程时子进程变成僵尸进程
static void NgxProcessGetStatus(void)
{
    pid_t   pid = -1;
    int     status = 0, error_number = 0, one = 0;   // 变量one用来标记信号是否已经过正常处理

    // 当worker子进程被kill时，父进程会收到SIGCHLD信号
    for(;;)
    {
        // 使用waitpid获取子进程的终止状态，避免子进程成为僵尸进程
        // 第一次waitpid返回一个0值，表示成功
        // 第二次调用waitpid会返回一个0(只有waitpid的第三个参数为WNOHANG时waitpid才会返回0), 返回的这个0表示worker子进程并不是立即可用的，waitpid不阻塞，立即返回0
        pid = waitpid(-1, &status, WNOHANG);    // 第一个参数为-1，表示等待任何子进程
                                                // 第二个参数：保存子进程的状态信息
                                                // 第三个参数：提供额外选项，WNOHANG表示不要阻塞，让这个waitpid()立即返回
        
        if(0 == pid)
        {
            return;
        }

        if(-1 == pid)       // 表示这个waitpid调用有错误
        {
            // 需要打印一些日志
            error_number = errno;

            if(EINTR == error_number)       // 当进程执行阻塞式系统调用时,被中断, 系统调用会被立即终止并设置errno为EINTR
            {
                continue;
            }

            if(ECHILD == error_number)
            {
                if(one)     // 没有子进程
                {
                    return;
                }
                
                NgxLogErrorCore(NGX_LOG_INFORMATION, error_number, "waitpid() failed!");
                return;
            }
            
            NgxLogErrorCore(NGX_LOG_ALERT, error_number, "waitpid() failed!");
            return;
        }

        // 执行到这里表示成功处理子进程
        one = 1;        // 标记waitpid()返回了正常的返回值

        if(WTERMSIG(status))        // 获取使子进程终止的信号编号
        {
            NgxLogErrorCore(NGX_LOG_ALERT, 0, "pid = %P exited on signal %d!", pid, WTERMSIG(status));
        }
        else
        {
            // WEXITSTATUS()获取子进程传递给exit或者_exit参数的低八位
            NgxLogErrorCore(NGX_LOG_NOTICE, 0, "pid = %P exited with code %d!", pid, WEXITSTATUS(status));
        }
    }
}
