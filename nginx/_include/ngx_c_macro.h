#ifndef __NGX_C_MACRO_H__
#define __NGX_C_MACRO_H__

#define NGX_MAX_ERROR_STRING 2048           //显示的错误信息最大数组长度

// 数字相关宏定义
#define NGX_INT64_LENGTH (sizeof("-9223372036854775808") - 1)
#define NGX_MAX_UINT32_VALUE (uint32_t) 0xffffffff

// 简单功能函数
// 常规memcpy返回的是指向目标dst的指针
// ngx_cpymem返回的是目标【拷贝数据后】的终点位置，连续复制多段数据时方便
#define NgxCopyMemory(dst, src, n) (((u_char *) memcpy(dst, src, n)) + (n))

// 日志相关宏定义
#define NGX_LOG_STANDARD_ERROR 0    // 控制台错误【stderr】：最高级别日志，日志的内容不再写入log参数指定的文件，而是会直接将日志输出到标准错误设备比如控制台屏幕
#define NGX_LOG_EMERGENCY 1
#define NGX_LOG_ALERT 2
#define NGX_LOG_CRITICAL 3
#define NGX_LOG_ERROR 4
#define NGX_LOG_WARNING 5
#define NGX_LOG_NOTICE 6
#define NGX_LOG_INFORMATION 7
#define NGX_LOG_DEBUG 8             // 调试 【debug】：最低级别

#define NGX_ERROR_LOG_PATH "logs/err.log"

#endif