#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
#include <vector>
#endif

#include "ngx_c_conf.h"
#include "ngx_c_func.h"

/*
 * 定义常量宏
 */
#define FILECHARNUMBER 1024

CConfig* CConfig::m_instance = NULL;

CConfig::CConfig()
{
        // TODO:
}

CConfig::~CConfig()
{
        std::vector<PCConfItem>::iterator pos;
        for(pos = m_config_item_list.begin(); pos != m_config_item_list.end(); ++pos)
        {
                delete (*pos);
        }
        m_config_item_list.clear();
}

bool CConfig::Load(const char* p_conf_name)
{
        FILE* file_point;
        file_point = fopen(p_conf_name, "r");
        if(NULL == file_point)
        {
                return false;
        }

        // line_buffer用于存储读取的1行配置信息
        char line_buffer[FILECHARNUMBER];             // 1行配置信息不应当超过宏FILECHARNUMBER定义的字节数

        // 配置文件成功读取
        while(!feof(file_point))
        {
                if(NULL == fgets(line_buffer, FILECHARNUMBER, file_point))      // NULL代表读取到空字符
                {
                        continue;
                }

                if('\n' == line_buffer[0])      // 读取到空行
                {
                        continue;
                }

                // 处理注释
                if(true)
                {
                        // TODO:
                        continue;
                }

                // 读取到有效配置信息行
                // 需要去除读入的char数组中尾部的空格、回车\n、\t
                if(0 < strlen(line_buffer))
                {
                        while(line_buffer[strlen(line_buffer) - 1] ||
                                line_buffer[strlen(line_buffer) - 1] ||
                                line_buffer[strlen(line_buffer) - 1])
                        {
                                line_buffer[strlen(line_buffer) - 1] = '\0';
                        }
                }

                // 获取到已处理的有效配置信息字符数组
                // Example: ListenPort = 80
                char* p_temporarily = strchr(line_buffer, '=');
                if(NULL != p_temporarily)
                {
                       PCConfItem p_conf_item = new CConfItem();
                       memset(p_conf_item, 0, sizeof(CConfItem));

                       strncpy(p_conf_item->Item_Name, line_buffer, (int)(p_temporarily - line_buffer));
                       strcpy(p_conf_item->Item_Content, p_temporarily + 1);

                        RightTrim(p_conf_item->Item_Name);
                        RightTrim(p_conf_item->Item_Content);
                        LeftTrim(p_conf_item->Item_Name);
                        LeftTrim(p_conf_item->Item_Content);

                       m_config_item_list.push_back(p_conf_item);
                }
        }

        fclose(file_point);
        return true;
}
