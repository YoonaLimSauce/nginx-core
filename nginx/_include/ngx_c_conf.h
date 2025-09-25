#ifndef __NGX_C_CONF_H__
#define __NGX_C_CONF_H__

/*
 * 使用C++11标准提供的call_once函数的单例模式创建CConfig类处理配置文件
 */
class CConfig
{
private:
	CConfig();
	static CConfig* m_instance;
	static std::once_flag init_Flag;

public:
	~CConfig();
	static CConfig& Get_Instance()
	{
		std::call_once(init_Flag, [](){
			m_instance = new CConfig();	
		})	
		return *m_instance;
	}
}

CConfig* CConfig::m_instance = nullptr;
std::once_flag CConfig::init_Flag;

#endif

