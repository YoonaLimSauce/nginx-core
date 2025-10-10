#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ngx_c_conf.h"
#include "ngx_c_signal.h"
#include "ngx_c_func.h"

// 设置标题有关的全局变量
char** global_os_argv = NULL;           //原始命令行参数数组,在main中会被赋值
char* globalp_environ_memory = NULL;   //指向自己分配的env环境变量的内存
int global_environ_length = 0;          //环境变量所占内存大小

int main(int argc, char* const* argv)
{
    global_os_argv = (char**) argv;

    NgxInitializeProcessTitle();    // 拷贝环境变量参数信息到新内存地址

    CConfig* p_config = CConfig::GetInstance();     // 单例
    if(false == p_config->Load("nginx.conf"))
    {
        printf("load config file failed\n");
        exit(1);
    }

    NgxSetProcessTitle("nginx: master process");   // 设置进程标题

    while(1)
    {
        sleep(1);
    }

    return 0;
}
