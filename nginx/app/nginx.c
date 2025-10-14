#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ngx_c_conf.h"
#include "ngx_c_signal.h"
#include "ngx_c_func.h"
#include "ngx_c_global.h"

// 定义配置文件名称宏
#define NGX_CONF_FILE "nginx.conf"

// main函数中会调用的函数
static void FreeResource();

// 设置标题有关的全局变量
char** global_os_argv = NULL;           //原始命令行参数数组,在main中会被赋值
char* globalp_environ_memory = NULL;   //指向自己分配的env环境变量的内存
int global_environ_length = 0;          //环境变量所占内存大小

// 进程本身有关的全局变量
pid_t ngx_pid = -1;      // 当前进程的pid

int main(int argc, char* const* argv)
{
    int exit_code = 0;      // 退出状态码, 0表示正常退出

    ngx_pid = getpid();
    global_os_argv = (char**) argv;
 
    // 配置文件读取，初始化使用，先读取配置
    CConfig* p_config = CConfig::GetInstance();
    p_config->m_config_item_list.clear();      // 单例
    if(false == p_config->Load(NGX_CONF_FILE))
    {
        NgxLogStandardError(0, "配置文件[%s]载入失败, 退出!", NGX_CONF_FILE);
        /*
        * exit(1);终止进程，在main中出现和return效果一样
        * exit(0)表示程序正常
        * exit(1)/exit(-1)表示程序异常退出
        * exit(2)表示表示系统找不到指定的文件
        */
       exit_code = 2;
       
       FreeResource();
       printf("程序退出, 在家!\nProgram exit, exit code: %d\n", exit_code);
       return exit_code;
    }
    
    // 初始化函数集合
    NgxLogInitialize();    // 初始化日志模块(创建/打开日志文件)
    
    NgxInitializeProcessTitle();    // 拷贝环境变量参数信息到新内存地址
    NgxSetProcessTitle("nginx: master process");   // 设置进程标题

    while(1)
    {
        sleep(3600);
    }

    FreeResource();
    printf("程序退出, 在家!\nProgram exit, exit code: %d\n", exit_code);
    return exit_code;
}

void FreeResource()
{
    // 因为设置可执行程序标题导致的环境变量分配的内存，我们应该释放
    if(globalp_environ_memory)
    {
        delete[] globalp_environ_memory;
        globalp_environ_memory = NULL;
    }

    // 关闭日志文件
    if(STDERR_FILENO != ngx_log.Fd && ngx_log.Fd != -1)
    {
        close(ngx_log.Fd);  // 直接关闭日志文件
        ngx_log.Fd = -1;    // 标记，防止被再次close
    }
}