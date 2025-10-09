#ifndef __NGX_C_CONF_H__
#define __NGX_C_CONF_H__

#include <vector>

#include <pthread.h>
#include <stdlib.h>

#include "ngx_c_global.h"

#ifndef __cplusplus
#include <stdbool.h>
#endif

/*
 * 采用单例模式创建CConfig()类用于处理配置文件
 */
class CConfig
{
private:
	CConfig();
	static CConfig* m_instance;

public:
	~CConfig();

	// 内部类自动析构单例模式创建的单例变量
	class CConfigDestructor
	{
	public:
		~CConfigDestructor()
		{
			if(NULL !=CConfig::m_instance)
			{
				delete CConfig::m_instance;
				CConfig::m_instance = NULL;
			}
		}
	};

	// 单例模式创建获取配置的单例变量指针
	static CConfig* Get_Instance()
	{
		// 加互斥锁
		pthread_mutex_t lock;

		int init_result = pthread_mutex_init(&lock, NULL);
		if(init_result != 0)
		{
			perror("pthread互斥锁初始化失败!");
			exit(EXIT_FAILURE);
		}

		if(m_instance == NULL)
		{
			pthread_mutex_lock(&lock);
			
			if(m_instance == NULL)
			{
				m_instance = new CConfig();
				static CConfigDestructor destructor;
			}
			
			pthread_mutex_unlock(&lock);
		}

		// 释放互斥锁
		pthread_mutex_destroy(&lock);

		return m_instance;
	}

	// 加载配置文件的函数
	bool Load(const char* p_conf_name);

	// 获取配置项的值
	const char* GetString(const char* p_item_name);

	// 获取配置项的值，如果不存在则返回默认值
	int GetIntDefault(const char* p_item_name, int p_default_value);

	// 存储配置信息的vector容器
	std::vector<PCConfItem> m_config_item_list;
};

#endif
