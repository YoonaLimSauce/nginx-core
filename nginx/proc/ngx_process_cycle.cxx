#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "ngx_func.h"
#include "ngx_macro.h"
#include "ngx_c_conf.h"

// 函数声明
static void     NgxStartWorkerProcesses(int initial_numbers);
static int      NgxSpawnProcess(int initial_numbers, const char* point_process_name);
static void     NgxWorkerProcessCycle(int initial_number, const char* point_process_name);
static void     NgxWorkerProcessInitialize(int initial_number);

// 局部文件变量声明
static u_char master_process[] = "master process";

// 创建worker子进程
void NgxMasterProcessCycle()
{
    // (1)  阻塞信号
    sigset_t set;       // 信号集
    sigemptyset(&set);  // 清空信号集

    // 下列这些信号在执行本函数期间不希望收到（保护不希望由信号中断的代码临界区）
    // 这些信号会被阻塞, 阻塞期间, 会被合并为一个信号, 等待取消信号屏蔽后会收到信号
    sigaddset(&set, SIGCHLD);       // 子进程状态改变
    sigaddset(&set, SIGALRM);       // 定时器超时
    sigaddset(&set, SIGIO);         // 异步I/O
    sigaddset(&set, SIGINT);        // 终端断开符
    sigaddset(&set, SIGHUP);        // 连接断开
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);       
    sigaddset(&set, SIGWINCH);      // 终端窗口大小改变
    sigaddset(&set, SIGTERM);       // 终止
    sigaddset(&set, SIGQUIT);       // 终端退出符
    // ...... TO BE ADDED ......

    if(-1 == sigprocmask(SIG_BLOCK, &set, NULL))        // //SIG_BLOCK参数表示 设置 进程 新的信号屏蔽字 为 “当前信号屏蔽字 和 第二个参数指向的信号集的并集
    {
        NgxLogErrorCore(NGX_LOG_ALERT, errno, "Ngx MasterProcessCycle()中sigpromark()失败!\n");
    }       // 即使sigprocmask失败，程序流程 也继续往下走

    // (2)  设置主进程标题
    size_t size = sizeof(master_process) + global_argv_need_memory;     // sizeof 包括字符串末尾的\0

    if(1000 > size)
    {
        char title[1000] = {0};
        strcpy(title, (const char*) master_process);
        strcat(title, " ");

        for(int i = 0; i < global_os_argc; i++)
        {
            strcat(title, global_os_argv[i]);
        }

        NgxSetProcessTitle(title);
        NgxLogErrorCore(NGX_LOG_NOTICE, 0, "%s %P 启动并开始运行......!", title, ngx_pid);      // 设置标题时同时记录下来进程名，进程id等信息到日志
    }

    // (3)  从配置文件中读取要创建的worker进程数量
    CConfig* point_configuration = CConfig::GetInstance();     // 单例类
    int work_process = point_configuration->GetIntDefault("WorkProcesses", 1);     // 从配置文件中得到要创建的worker进程数量
    NgxStartWorkerProcesses(work_process);      // 创建worker子进程

    // 创建子进程后，父进程的执行流程会到这里，子进程不会
    sigemptyset(&set);      // 信号屏蔽字为空，表示不屏蔽任何信号

    for(;;)
    {
        NgxLogErrorCore(0, 0, "这是父进程, pid为%P", ngx_pid);
            // 1)   根据给定的参数设置新的mask并阻塞当前进程【因为是个空集，所以不阻塞任何信号】
            // 2)   一旦收到信号，便恢复原先的信号屏蔽【阻塞了10个信号，保证执行流程不会再次被其他信号截断】
            // 3)   调用该信号对应的信号处理函数
            // 4)   信号处理函数返回后，sigsuspend返回，使程序流程继续往下走
        sigsuspend(&set);       // 阻塞，等待信号，此时进程是挂起的，不占用cpu时间，只有收到信号才会被唤醒（返回）
    }
}

// 描述：根据给定的参数创建指定数量的子进程
// initial_numbers: 要创建的子进程数量
static void NgxStartWorkerProcesses(int initial_numbers)
{
    for(int i = 0; i < initial_numbers; i++)     // master进程创建子进程
    {
        NgxSpawnProcess(i, "worker process");
    }
}

// 描述：Fork一个子进程
// initial_numbers：进程编号【0开始】
// point_process_name：子进程名字"worker process"
static int NgxSpawnProcess(int initial_numbers, const char* point_process_name)
{
    pid_t pid = -1;
    pid = fork();       // fork()系统调用产生子进程

    switch(pid)         // 根据父子进程，分支处理
    {
        case -1:        // Fork子进程失败
            NgxLogErrorCore(NGX_LOG_ALERT, errno, "NgxSpawnProcess() fork() 产生子进程 num=%d, procname=\"%s\"失败!", initial_numbers, point_process_name);
            return -1;

        case 0:         // 子进程分支
            ngx_parent = ngx_pid;       // 子进程中，所有原来的pid变成了父pid
            ngx_pid = getpid();         // 重新获取pid, 即本子进程的pid
            NgxWorkerProcessCycle(initial_numbers, point_process_name);
            break;

        default:        // 父进程分支，直接break;
            break;
    }

    return pid;
}

// worker子进程的功能函数
// 每个woker子进程, 子进程分叉才会走到这里（无限循环【处理网络事件和定时器事件以对外提供web服务】）
// initial_number：进程编号【0开始】
static void NgxWorkerProcessCycle(int initial_number, const char* point_process_name)
{
    ngx_process = NGX_PROCESS_WORKER;       // 设置进程的类型，是worker进程

    // 重新为子进程设置进程名，不要与父进程重复
    NgxWorkerProcessInitialize(initial_number);     
    NgxSetProcessTitle(point_process_name);         // 设置标题
    NgxLogErrorCore(NGX_LOG_NOTICE, 0, "%s %P 启动并开始运行......!", point_process_name, ngx_pid);     // 设置标题时同时记录下来进程名，进程id信息到日志

    // 无限循环
    // setvbuf(stdout,NULL,_IONBF,0);   setvbuf函数直接将printf缓冲区禁止
    for(;;)
    {
        sleep(1000);
    }
}

// 描述：子进程创建时初始化
static void NgxWorkerProcessInitialize(int initial_number)
{
    sigset_t set;           // 信号集
    sigemptyset(&set);      // 清空信号集

    if(-1 == sigprocmask(SIG_SETMASK, &set, NULL))
    {
        NgxLogErrorCore(NGX_LOG_ALERT, errno, "NgxWorkerProcessInitialize()中sigprocmask()失败!");
    }

    // TODO: nothing to decide now
}
