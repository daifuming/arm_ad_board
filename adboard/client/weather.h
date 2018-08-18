#ifndef WEATHER_H
#define WEATHER_H

typedef struct 
{
	
	char temp[16];			//温度
	char wd[16];			//风向
	char ws[16];			//风速
	char sd[16];			//湿度
	char ap[16];			//气压
	char city[128]; 		//城市
}Weather;


/**
 * 天气线程函数
 * @param  arg 需要查询的城市id(char*)
 * @return     NULL
 */
void *weather(void *arg);


#endif