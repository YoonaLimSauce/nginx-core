#ifndef __NGX_C_GLOBAL_H__
#define __NGX_C_GLOBAL_H__

/*
 * 配置文件存取结构体定义
 * Item_Name 存储配置项目名
 * Item_Content 存储配置项目名对应的设置项信息
 * CConfItem 为结构体别名
 * LPConfItem 为结构体指针别名
 */
typedef struct
{
	char Item_Name[50];
	char Item_Content[500];
}CConfItem, *PCConfItem;

#endif
