#ifndef __NGX_C_CONF_H__
#define __NGX_C_CONF_H__

#include <pthread.h>
#include <stdlib.h>
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
	static CConfig* Get_Instance()
	{
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
			}
			
			pthread_mutex_unlock(&lock);
		}
		pthread_mutex_destroy(&lock);
	}
	return m_instance;
}

#endif
