#ifndef __NGX_C_GLOBAL_H__
#define __NGX_C_GLOBAL_H__

#include <signal.h>

/*
 * 配置文件存取结构体定义
 * Item_Name 存储配置项目名
 * Item_Content 存储配置项目名对应的设置项信息
 * CConfItem 为结构体别名
 * LPConfItem 为结构体指针别名
 */
typedef struct
{
	char Item_Name[64];
	char Item_Content[192];
} CConfItem, *PCConfItem;

// 日志结构体
typedef struct 
{
	int Log_Level;		// 日志级别
	int Fd;				// 日志文件描述符
} ngx_log_type;


// 外部全局量声明
// 用于配置Nginx进程名称
extern char** 	global_os_argv;
extern char* 	global_point_environment_memory;
extern int 		global_os_argc;
extern int		global_daemonized;
extern size_t 	global_argv_need_memory;
extern size_t 	global_environment_need_memory;

extern int				ngx_process;
extern pid_t 			ngx_pid;
extern pid_t 			ngx_parent;
extern sig_atomic_t		ngx_reap;
extern ngx_log_type 	ngx_log;

#endif
