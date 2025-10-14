#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>

#include "ngx_c_conf.h"
#include "ngx_c_global.h"
#include "ngx_c_func.h"
#include "ngx_c_macro.h"

// 全局变量, 日志等级
static const u_char error_levels[][20] = 
{
    {"standard error"},
    {"emergent error"},
    {"alert error"},
    {"critical error"},
    {"error"},
    {"warning"},
    {"notice"},
    {"information"},
    {"debug"}
};
ngx_log_type ngx_log;

void NgxLogStandardError(int error, const char* fmt, ...)
{
    va_list args;
    u_char *p = NULL;
    u_char error_string[NGX_MAX_ERROR_STRING + 1];
    memset(error_string, 0, NGX_MAX_ERROR_STRING + 1);          // 字符串初始化为0

    u_char* const last = error_string + NGX_MAX_ERROR_STRING;                    // last指向整个buffer最后去了【指向最后一个有效位置的后面也就是非有效位】，作为一个标记，防止输出内容溢出
    p = NgxCopyMemory(error_string, "nginx: ", 7);            // p指向"nginx: "之后

    va_start(args, fmt);        // 使args指向起始的参数
    p = NgxVariableStringLengthPrintf(p, last, fmt, args);      // 组合出错误信息字符串保存在error_string里
    va_end(args);               // 释放args

    if (error)      // 如果错误代码不是0，表示有错误发生
    {
        // 错误代码和错误信息要显示出来
        p = NgxLogErrorNumber(p, last, error);      
    }

    p = ((last) <= p) ? last - 1 : p;
    *p++ = '\n';
    *p = '\0';

    write(STDERR_FILENO, error_string, p - error_string);   // 输出错误信息到标准错误输出
}

// 描述：给一段内存，一个错误编号，我要组合出一个字符串，形如：   (错误编号: 错误原因)，放到给的这段内存中去
// buf：是个内存，要往这里保存数据
// last：放的数据不要超过这里
// error_number：错误编号，我们是要取得这个错误编号对应的错误字符串，保存到buffer中
u_char* NgxLogErrorNumber(u_char* buf, u_char* const last, int error_number)
{
    char* p_error_information = strerror(error_number); // 根据资料不会返回NULL
    size_t length = strlen(p_error_information);

    // 插入一些自定义的字符串
    char left_string[10] = {0};
    sprintf(left_string, " (%d: ", error_number);
    size_t left_length = strlen(left_string);

    char right_string[] = ") ";
    size_t right_length = strlen(right_string);

    size_t extra_length = left_length + right_length;

    // 保证整个都能装得下，否则就全部抛弃
    // nginx的做法是 如果位置不够，就硬留出50个位置【哪怕覆盖掉以往的有效内容】
    if((buf + extra_length + length) < last)
    {
        buf = NgxCopyMemory(buf, left_string, left_length);
        buf = NgxCopyMemory(buf, p_error_information, length);
        buf = NgxCopyMemory(buf, right_string, right_length);
    }

    return buf;
}

// 往日志文件中写日志，代码中有自动加换行符
// 日志定向为标准错误，则直接往屏幕上写日志【比如日志文件打不开，则会直接定位到标准错误，此时日志就打印到屏幕上】
// level: 日志等级
// err：错误代码，如果不是0，就应该转换成显示对应的错误信息,一起写到日志文件中
void NgxLogErrorCore(int level, int err, const char* fmt, ...)
{
    u_char error_string[NGX_MAX_ERROR_STRING + 1];
    memset(error_string, 0, NGX_MAX_ERROR_STRING + 1);
    u_char* last = error_string + NGX_MAX_ERROR_STRING;

    struct timeval time_value;
    struct tm time_information;
    time_t second;
    u_char* p = NULL;       // 指向当前要拷贝数据到其中的内存位置

    memset(&time_value, 0, sizeof(struct timeval));
    memset(&time_information, 0, sizeof(struct tm));

    gettimeofday(&time_value, NULL);        // 获取当前时间，返回自1970-01-01 00:00:00到现在经历的秒数【第二个参数是时区】

    second = time_value.tv_sec;
    localtime_r(&second, &time_information);   // 将时间戳second转换为本地时间time_information，带_r的是线程安全的版本
    time_information.tm_mon++;   // 月份从0开始，所以要加1
    time_information.tm_year += 1900;   // 年份从1900开始，所以要加1900

    u_char string_current_time[40] = {0};       // 组合出一个当前时间字符串，格式形如：2025/10/15 00:25:56
    NgxStringLengthPrintf(string_current_time, (u_char*) -1, "%4d/%02d/%02d %02d:%02d:%02d", time_information.tm_year, time_information.tm_mon, time_information.tm_mday, time_information.tm_hour, time_information.tm_min, time_information.tm_sec);

    p = NgxCopyMemory(error_string, string_current_time, strlen((const char*) string_current_time));     
    p = NgxStringLengthPrintf(p, last, " [%s] ", error_levels[level]);
    p = NgxStringLengthPrintf(p, last, "%P: ", ngx_pid);
    
    va_list args;
    va_start(args, fmt);
    p = NgxVariableStringLengthPrintf(p, last, fmt, args);
    va_end(args);

    if(err)     // 如果错误代码不是0，表示有错误发生
    {
        // 错误代码和错误信息要显示出来
        p = NgxLogErrorNumber(p, last, err);
    }

    // 若位置不够，那换行也要硬插入到末尾，哪怕覆盖到其他内容
    if(p >= (last - 1))
    {
        p = last - 2;   // 把尾部空格留出来, 增加个换行符
    }
    *p++ = '\n';

    ssize_t n = -1;
    while(1)
    {
        // 日志的等级高, 应当直接打印在标准输出到屏幕上
        if(level > ngx_log.Log_Level)
        {
            break;
        }

        // 写日志文件 
        n = write(ngx_log.Fd, error_string, p - error_string);
        if(-1 == n)
        {
            // 写日志文件失败，排查失败原因
            if(ENOSPC == errno)
            {
                // 磁盘没空间了
                // TODO: 处理磁盘满的故障
            }
            else
            {
                // 其他错误
                // 把这个错误显示到标准错误设备
                if(STDERR_FILENO != ngx_log.Fd)
                {
                    n = write(STDERR_FILENO, error_string, p - error_string);   // 写到标准错误输出
                }
            }
        }

        break;
    }
}

// 日志初始化，把日志文件打开
// 但是这里涉及到释放的问题，需要专门处理
void NgxLogInitialize()
{
    u_char* p_log_name = NULL;
    size_t error_log_length = 0;
    
    CConfig* p_config = CConfig::GetInstance();
    p_log_name = (u_char*) p_config->GetString("Log");
    // 配置文件中未定义Log日志文件路径，需要定义缺省的路径文件名
    if (NULL == p_log_name)
    {
        p_log_name = (u_char*) NGX_ERROR_LOG_PATH;  // 默认为"logs/error.log"
    }
    ngx_log.Log_Level = p_config->GetIntDefault("LogLevel", NGX_LOG_NOTICE);    // 缺省日志等级为6【注意】，如果读失败，就给缺省日志等级

    error_log_length = strlen((const char*)p_log_name);

    // 如果不存在日志文件，则创建日志文件
    // 如果日志文件存在，则打开日志文件，并追加日志
    // 如果日志文件存在，但无内容，则以读写方式打开日志文件
    ngx_log.Fd = open((const char*)p_log_name, O_WRONLY | O_APPEND | O_CREAT, 0644);

    if(-1 == ngx_log.Fd)
    {
        NgxLogStandardError(errno, "[alert] could not open error log file: open() \"%s\" failed", p_log_name);
        ngx_log.Fd = STDERR_FILENO;     // 直接定位到标准错误
    }
}