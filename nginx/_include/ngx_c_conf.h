#ifndef __NGX_C_CONF_H__
#define __NGX_C_CONF_H__

/*
 * 采用C++11标准支持的局部静态变量的单例模式创建CConfig()类处理类配置文件
 */
class CConfig
{
private:
        CConfig();

        CConfig(const CConfig&) = delete;
        CConfig& operator = (const CConfig&) = delete;

public:
        ~CConfig();
        static CConfig& Get_Instance()
        {
                static CConfig instance;
                return instance;
        }
}

#endif