#include <stdio.h>
#include <string.h>

// 截取字符串尾部空格
void RightTrim(char *string)
{
    if(NULL == string)
    {
        return;
    }

    size_t len = strlen(string);
    while(len > 0 && string[len-1] ==' ')
    {
        len--;
    }
    string[len] = '\0';
}

// 截取字符串首部空格
void LeftTrim(char *string)
{
    if(' ' != string[0])
    {
        return;
    }

    size_t len = strlen(string);
    char* p_tmp = string;
    while('\0' != *p_tmp && ' ' == *p_tmp)
    {
        p_tmp++;
    }
    if('\0' == *p_tmp)  // 全部是空格
    {
        *string = '\0';
    }

    char* p_tmp2 = string;
    while('\0' != *p_tmp)
    {
        *(p_tmp2++) = *(p_tmp++);
    }
    *p_tmp2 = '\0';
}