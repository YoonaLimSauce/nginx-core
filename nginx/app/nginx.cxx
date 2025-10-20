#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ngx_c_conf.h"     // 和配置文件处理相关的类,名字带c_表示和类有关
#include "ngx_func.h"       // 函数声明

// 定义配置文件名称宏
#define NGX_CONF_FILE "nginx.conf"

// main函数中会调用的函数
static void FreeResource();

// 设置标题有关的全局变量
char**  global_os_argv = NULL;           // 原始命令行参数数组,在main中会被赋值
char*   global_point_environment_memory = NULL;   // 指向自己分配的env环境变量的内存
int     global_os_argc = 0;                 // 参数个数
int     global_daemonized = 0;          // 守护进程标记，标记是否启用了守护进程模式，0：未启用，1：启用
size_t  global_argv_need_memory = 0;          // 保存下argv参数所需要的内存大小
size_t  global_environment_need_memory = 0;  // 环境变量所占内存大小

// 进程本身有关的全局变量
pid_t           ngx_pid = -1;           // 当前进程的pid
pid_t           ngx_parent = -1;        // 父进程的pid
int             ngx_process = -1;       // 进程类型: master进程, worker进程
sig_atomic_t    ngx_reap = -1;          // 标记子进程状态变化[一般是子进程发来SIGCHLD信号表示退出]

int main(int argc, char* const* argv)
{
    int exit_code = 0;      // 退出状态码, 0表示正常退出

    // (1)  不需要释放内存的放最上边
    ngx_pid = getpid();         // 进程pid
    ngx_parent = getppid();     // 父进程的id 

    global_argv_need_memory = 0;
    int i = 0;
    for(i = 0; i < argc; ++i)
    {
        global_argv_need_memory += strlen(argv[i]) + 1;
    }       // 统计argv所占的内存
    for(i = 0; environ[i]; ++i)
    {
        global_environment_need_memory += strlen(environ[i]) + 1;
    }       // 统计环境变量所占的内存

    global_os_argc = argc;              // 保存参数个数
    global_os_argv = (char**) argv;     // 保存参数指针

    // (2)  配置文件必须最先要，后边一切初始化操作都需要配置文件中的参数
    CConfig* point_config = CConfig::GetInstance();     // 单例
    point_config->m_config_item_list.clear(); 
    if(false == point_config->Load(NGX_CONF_FILE))
    {
        NgxLogStandardError(0, "配置文件[%s]载入失败, 退出!", NGX_CONF_FILE);
        /*
        * exit(1);终止进程，在main中出现和return效果一样
        * exit(0)表示程序正常
        * exit(1)/exit(-1)表示程序异常退出
        * exit(2)表示表示系统找不到指定的文件
        */
        exit_code = 2;

        goto label_exit;
    }

    // (3)  初始化函数集中在这里执行
    NgxLogInitialize();    // 初始化日志模块(创建/打开日志文件)
    if(0 != NgxInitializeSignals())
    {
        exit_code = 1;
        
        goto label_exit;
    }
    NgxInitializeProcessTitle();    // 拷贝环境变量参数信息到新内存地址
    
    // (4)  初始化内存环境后伴随执行、在主任务前应当执行的一系列函数在这里执行
    // NgxSetProcessTitle("nginx: master process");   // 设置进程标题, 该函数被移入 NgxInitializeSignals() 函数中执行

    // (5)  无法归类的必要执行函数

    // (6)  创建守护进程
    if(1 == point_config->GetIntDefault("Daemon", 0))       // 配置文件中关守护进程的配置信息
    {
        int create_daemon_result = NgxDaemon();

        if(-1 == create_daemon_result)      // fork()失败
        {
            exit_code = -1;     // 标记失败
            
            goto label_exit;
        }

        if(1 == create_daemon_result)
        {
            // 原始的父进程
            FreeResource();     // 正常fork()守护进程后的正常退出

            exit_code = 0;
            return exit_code;
        }
        global_daemonized = 1;      // 守护进程标记
    }

    // (5)  开始正式的主工作流程，主流程一致在下边这个函数里循环，暂时不会跳出循环
    NgxMasterProcessCycle();        // 父进程、子进程，正常工作期间都在这个函数里循环

label_exit:
    NgxLogStandardError(0, "程序退出, 再见了!");
    FreeResource();
    return exit_code;
}

void FreeResource()
{
    // 因为设置可执行程序标题导致的环境变量分配的内存，我们应该释放
    if(global_point_environment_memory)
    {
        delete[] global_point_environment_memory;
        global_point_environment_memory = NULL;
    }

    // 关闭日志文件
    if(STDERR_FILENO != ngx_log.Fd && ngx_log.Fd != -1)
    {
        close(ngx_log.Fd);  // 直接关闭日志文件
        ngx_log.Fd = -1;    // 标记，防止被再次close
    }
}