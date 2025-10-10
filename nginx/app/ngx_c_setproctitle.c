#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ngx_c_global.h"

void NgxInitializeProcessTitle()
{
    int i = 0;
    for (i = 0; environ[i]; i++)
    {
        global_environ_length += strlen(environ[i]) + 1;
    }

    //这里无需判断globalp_environ_memory == NULL,有些编译器new会返回NULL，有些会报异常，但不管怎样，如果在重要的地方new失败了，你无法收场，让程序失控崩溃，助你发现问题为好； 
    globalp_environ_memory = new char[global_environ_length];
    memset(globalp_environ_memory, 0, global_environ_length);

    // 将environ指针指向的环境变量参数信息拷贝到globalp_environ_memory中
    char* p_tmp = globalp_environ_memory;
    for (i = 0; environ[i]; i++)
    {
        size_t length = strlen(environ[i]) + 1;
        strcpy(p_tmp, environ[i]);
        environ[i] = p_tmp;
        p_tmp += length;
    }

    p_tmp = NULL;
}

void NgxSetProcessTitle(const char* title)
{
    size_t title_length = strlen(title) + 1;

    size_t environ_length = 0;
    for (int i = 0; global_os_argv[i]; i++)
    {
        environ_length += strlen(global_os_argv[i]) + 1;
    }

    if(title_length > global_environ_length + environ_length)
    {
        return;
    }

    // 设置后续的命令行参数为空，表示只有argv[]中只有一个元素了
    // 防止后续argv被滥用，因为很多判断是用argv[] == NULL来做结束标记判断的
    global_os_argv[1] = NULL;

    char* p_tmp = global_os_argv[0];
    strcpy(p_tmp, title);
    p_tmp += title_length;

    memset(p_tmp, 0, global_environ_length + environ_length - title_length);

    p_tmp = NULL;
}