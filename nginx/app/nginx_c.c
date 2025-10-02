#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "ngx_c_conf.h"
#include "ngx_c_func.h"

CConfig* CConfig::m_instance = NULL;

CConfig::CConfig()
{
	\\ TODO: 
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

}
