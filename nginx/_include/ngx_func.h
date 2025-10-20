#ifndef __NGX_C_FUNC_H__
#define __NGX_C_FUNC_H__

// 字符串处理函数
void    RightTrim(char* string);
void    LeftTrim(char* string);

// 设置可执行程序标题相关函数
void    NgxInitializeProcessTitle();
void    NgxSetProcessTitle(const char* title);

// 日志，打印输出有关函数
void        NgxLogInitialize();
void        NgxLogStandardError(int error, const char* fmt, ...);
void        NgxLogErrorCore(int level, int err, const char* fmt, ...);
u_char*     NgxLogErrorNumber(u_char* buf, u_char* last, int error_number);
u_char*     NgxStringLengthPrintf(u_char* buf, u_char* last, const char* fmt, ...);
u_char*     NgxVariableStringLengthPrintf(u_char* buf, u_char* last, const char* fmt, va_list args);

// 信号、进程相关函数
int     NgxInitializeSignals();
int     NgxDaemon();
void    NgxMasterProcessCycle();
        
#endif