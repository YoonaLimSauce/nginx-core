#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ngx_global.h"

void NgxInitializeProcessTitle()
{
    //这里无需判断global_point_environment_memory == NULL,有些编译器new会返回NULL，有些会报异常，但不管怎样，如果在重要的地方new失败了，你无法收场，让程序失控崩溃，助你发现问题为好； 
    global_point_environment_memory = new char[global_environment_need_memory];
    memset(global_point_environment_memory, 0, global_environment_need_memory);

    // 将environ指针指向的环境变量参数信息拷贝到global_point_environment_memory中
    char* p_tmp = global_point_environment_memory;
    for (int i = 0; environ[i]; i++)
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
    // 假设，所有的命令 行参数我们都不需要用到了，可以被随意覆盖
    // 标题长度，不会长到原始标题和原始环境变量都装不下，否则怕出问题，不处理

    // (1)  计算新标题长度
    size_t title_length = strlen(title) + 1;

    if(title_length > global_argv_need_memory + global_environment_need_memory)
    {
        return;
    }

    // 设置后续的命令行参数为空，表示只有argv[]中只有一个元素了
    // 防止后续argv被滥用，因为很多判断是用argv[] == NULL来做结束标记判断的
    global_os_argv[1] = NULL;

    char* p_tmp = global_os_argv[0];
    strcpy(p_tmp, title);
    p_tmp += title_length;

    memset(p_tmp, 0, global_argv_need_memory + global_environment_need_memory - title_length);

    p_tmp = NULL;
}