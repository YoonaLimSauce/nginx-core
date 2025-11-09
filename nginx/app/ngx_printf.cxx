#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#include "ngx_global.h"
#include "ngx_macro.h"
#include "ngx_func.h"

static u_char* NgxStringPrintfNumber(u_char* buf, u_char* const last, uint64_t ui64, u_char zero, uintptr_t hexdecimal, uintptr_t width)
{
    u_char temp[NGX_INT64_LENGTH + 1];
    u_char *p = temp + NGX_INT64_LENGTH;
    size_t length = 0;
    uint32_t ui32 = 0;

    static const u_char hex[] = "0123456789abcdef", HEX[] = "0123456789ABCDEF";

    if(0 == hexdecimal)
    {
        do
        {
            *--p = (u_char) ('0' + ui64 % 10);
        } while (ui64 /= 10);
    }
    else if(1 == hexdecimal)
    {
        do
        {
            // 0xf就是二进制的1111，ui64 & 0xf，就等于把 一个数的最末尾的4个二进制位拿出来
            *--p = hex[(uint32_t) (ui64 & 0xf)];
        } while (ui64 >>= 4);   // 右移4位，就是除以16
    }
    else if(2 == hexdecimal)
    {
        do
        {
            *--p = HEX[(uint32_t) (ui64 & 0xf)];
        } while (ui64 >>= 4);
    }
    else
    {
        // TODO: hexdecimal 错误
        return NULL;
    }

    length = (temp + NGX_INT64_LENGTH) - p;     // // 得到数字的宽度

    while(width > length++ && last > buf)
    {
        *buf++ = zero;
    }

    length = (temp + NGX_INT64_LENGTH) - p;     // 得到数字的宽度

    if(last <= buf + length)
    {
        length = last - buf;
    }

    return NgxCopyMemory(buf, p, length);
}

u_char* NgxVariableStringLastPrintf(u_char* buf, u_char* const last, const char* fmt, va_list args)
{
    while(*fmt && buf < last)
    {
        u_char zero = 0;
        uintptr_t width = 0, sign = 0, hex = 0, fraction_width = 0, scale = 0, n = 0;
        int64_t i64 = 0;
        uint64_t ui64 = 0, fraction = 0;
        u_char* p = NULL;
        double f = 0.0;

        /*
         * fmt Conversion Specifier Summary:
         * %0:  使用0填充           (默认不使用0填充)
         * %d:  十进制整数          (默认输出十进制整数)
         * %u:  不带符号整数        (默认输出不带符号整数)
         * %.f:  浮点数             (默认输出整数)
         * %x:  小写十六进制整数    
         * %X:  大写十六进制整数    
         * %s:  字符串              (默认输出字符串)
         * %P:  pid_t类型           (默认输出进程号)
         */
        if('%' == *fmt)
        {
            zero = (u_char) (('0' == *++fmt) ? '0' : ' ');  // '0'表示%0
            width = 0;      // %1等非零数字, 仅对于%d, %.f等数字格式有效
            sign = 1;       // 默认为1表示有符号数, 仅%u表示无符号数
            hex = 0;        // 默认为0表示不以16进制形式显示, 1表示以16进制小写字母形式显示, 2表示以16进制大写字母形式显示
            fraction_width = 0;     // 小数点后数字位数, 配合%.f使用
            i64 = 0;        // %d的实际数字
            ui64 = 0;       // %ud的实际数字

            // while就是判断%后边是否是个数字
            while('0' <= *fmt && '9' >= *fmt)
            {
                // 计算数字的宽度
                width = 10 * width + (*fmt++ - '0');
            }   // end while('0' <= *fmt && '9' >= *fmt), 处理%后边接的字符是'0' --'9'之间的内容

            // 特殊格式需要标记特殊标记进行处理
            for( ; ; )
            {
                switch(*fmt)
                {
                    // %u，u表示无符号
                case 'u':
                    sign = 0;   // 标记无符号数
                    fmt++;
                    continue;
                
                    // %X，X表示十六进制，并且十六进制中的A-F以大写字母显示
                case 'X':
                    hex = 2;    // 标记以大写字母显示十六进制
                    sign = 0;
                    fmt++;
                    continue;

                    // %x，x表示十六进制，并且十六进制中的a-f以小写字母显示
                case 'x':
                    hex = 1;    // 标记以小写字母显示十六进制
                    sign = 0;
                    fmt++;
                    continue;

                    // 后边必须跟个数字，必须与%f配合使用，表示转换浮点数时小数部分的位数，不足位数则用0来填补
                case '.':
                    fraction_width = 0;
                    fmt++;
                    while('0' <= *fmt && '9' >= *fmt)
                    {
                        fraction_width = 10 * fraction_width + (*fmt++ - '0');
                    }   // end while('0' <= *fmt && '9' >= *fmt), 处理小数点后面的数字
                    break;
                
                default:
                    break;
                }   // end switch(*fmt)
                break;
            }   // 一些特殊的格式，我们做一些特殊的标记【给一些变量特殊值等等】

            switch(*fmt)
            {
            case 'd':       // 显示整型数据
                if(sign)
                {
                    i64 = (int64_t) va_arg(args, int);
                }
                else
                {
                    ui64 = (uint64_t) va_arg(args, u_int);
                }
                break;

            case 's':       // 显示字符串
                p = va_arg(args, u_char*);

                while(*p && last > buf)     // 字符串循环存入buffer缓冲区
                {
                    *buf++ = *p++;
                }
                fmt++;
                continue;

            case 'P':       // pid_t进程号
                i64 = (int64_t) va_arg(args, pid_t);
                sign = 1;
                break;

            case 'f':       // 显示double类型数据
                f = va_arg(args, double);

                if(0 > f)       // 负数
                {
                    *buf++ = '-';
                    f = -f;
                }
                
                ui64 = (int64_t) f;
                fraction = 0;

                // 要求小数点后显示指定位数的小数
                if(fraction_width)
                {
                    scale = 1;
                    for(n = 0; n < fraction_width; n++)
                    {
                        scale *= 10;
                    }

                    // 提取小数部分
                    fraction = (uint64_t) ((f - (double) ui64) * scale + 0.5);  // 四舍五入

                    if(fraction == scale)       // 进位
                    {
                        ui64++;
                        fraction = 0;
                    }
                }   // end if(fraction_width)

                // 显示整数部分
                buf = NgxStringPrintfNumber(buf, last, ui64, zero, hex, width);
                
                if(fraction_width)      // 指定了显示小数位数
                {
                    if(buf < last)
                    {
                        *buf++ = '.';   // 小数点
                    }
                    buf = NgxStringPrintfNumber(buf, last, fraction, '0', 0, fraction_width);       // 显示小数部分，位数不够的，前边填充'0'字符
                }

                fmt++;
                continue;

            default:
                *buf++ = *fmt++;
                continue;
            }   // end switch(*fmt)

            // 统一把显示的数字都保存到 ui64 里去；
            if(sign)
            {
                if(0 > i64)
                {
                    *buf++ = '-';
                    ui64 = (uint64_t) -i64;
                }
                else
                {
                    ui64 = (uint64_t) i64;
                }
            }   // end if(sign)

            // 调用 NgxStringPrintfNumber 显示整数数字
            buf = NgxStringPrintfNumber(buf, last, ui64, zero, hex, width);

            fmt++;
        }   //end if (*fmt == '%')
        else
        {
            *buf++ = *fmt++;
        }   //end if ('%' != *fmt)
    }   //end while (*fmt && buf < last)

    return buf;
}

u_char* NgxStringLastPrintf(u_char* buf, u_char* const last, const char* fmt, ...)
{
    va_list args;
    u_char* p = NULL;

    va_start(args, fmt);
    p = NgxVariableStringLastPrintf(buf, last, fmt, args);
    va_end(args);
    
    return p;
}

u_char* NgxStringLengthPrintf(u_char* buf, size_t max, const char* fmt, ...)
{
    u_char* p;
    va_list args;

    va_start(args, fmt);
    p = NgxVariableStringLastPrintf(buf, buf + max, fmt, args);
    va_end(args);

    return p;
}