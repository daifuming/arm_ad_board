#include "weather.h"
#include "cJSON.h"
#include "lcd_info.h"
#include "message.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define HTTP_REQ "GET %s HTTP/1.1\r\nHost:%s\r\nConnection: close\r\n\r\n"

/**
 * 分析天气json字符串
 * @param  json 天气json数据
 * @return      天气结构体
 */
static Weather json_parse(char *json)
{
	Weather weather;
	cJSON *root = cJSON_Parse(json);
	cJSON *weather_obj = cJSON_GetObjectItem(root, "weatherinfo");

	cJSON *obj = cJSON_GetObjectItem(weather_obj, "city");	//城市
	strcpy(weather.city, obj ->valuestring);

	obj = cJSON_GetObjectItem(weather_obj, "temp");	//温度
	strcpy(weather.temp, obj ->valuestring);
	strcat(weather.temp, "度");

	obj = cJSON_GetObjectItem(weather_obj, "WD");	//风向
	strcpy(weather.wd, obj ->valuestring);

	obj = cJSON_GetObjectItem(weather_obj, "WS");	//风速
	strcpy(weather.ws, obj ->valuestring);

	obj = cJSON_GetObjectItem(weather_obj, "SD");	//湿度
	strcpy(weather.sd, "湿度");
	strcat(weather.sd, obj ->valuestring);

	obj = cJSON_GetObjectItem(weather_obj, "AP");	//气压
	strcpy(weather.ap, obj ->valuestring);

	cJSON_Delete(root);
	return weather;
}

// typedef struct 
// {
// 	char city[128]; 	//城市
// 	char temp[8];		//温度
// 	char wd[8];			//风向
// 	char ws[16];		//风速
// 	char sd[16];			//湿度
// 	char ap[8];			//气压
// }Weather;

/**
 * 把天气信息在开发板上显示
 * @param weather 要显示的天气信息结构体
 */
void show_weather_lcd(Weather weather)
{
	struct LcdDevice* lcd = init_lcd("/dev/fb0");

	//(char *msg, int x, int y, int width, int height, struct LcdDevice *lcd, unsigned int color, font_x)
	show_weather(weather.city, 680, 0,   120, 50, lcd, 0x00ffffff, 10);
	show_weather(weather.temp, 680, 50,  120, 50, lcd, 0xff000000, 10);
	show_weather(weather.wd,   680, 100, 120, 50, lcd, 0x0000ff00, 10);
	show_weather(weather.ws,   680, 150, 120, 50, lcd, 0x00ff0000, 10);
	show_weather(weather.sd,   680, 200, 120, 50, lcd, 0x00f0f0f0, 10);
	show_weather(weather.ap,   680, 250, 120, 50, lcd, 0x000f0f00, 10);



	destroy_lcd(lcd);
}


void *weather(void *arg)
{
	char *city_id = (char *)arg;

	//连接服务器
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		perror("socket error");
	}
	//链接服务器
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	addr.sin_addr.s_addr = inet_addr("113.107.112.214");
	int ret = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
	if(ret < 0)
	{
		perror("链接服务器失败");
	}else
	{
		printf("链接服务器成功\n");
	}

	//生成请求字符串
	char request[1024] = {0};
	char req_file[150] = {0};
	sprintf(req_file, "/data/sk/%s.html", city_id);
	sprintf(request,HTTP_REQ,req_file,"www.weather.com.cn");
	
	//发送请求，接收数据并分析
	ret = write(sockfd, request, strlen(request));
	if (ret <= 0)
	{	perror("send error"); }
	char buf[1024] = {0};
	read(sockfd, buf, sizeof(buf));
	char *ptr = strstr(buf, "\r\n\r\n") + 4;
	ptr = strstr(ptr, "{");
	Weather weather = json_parse(ptr);

	char weather_buf[1024] = {0};
	sprintf(weather_buf, "%s:%s,%s,%s,%s,%s",  weather.city, weather.temp, weather.wd, weather.ws, weather.sd, weather.ap);
	printf("%s\n", weather_buf);
	//show_msg(weather_buf, 100*i);
	show_weather_lcd(weather);
	close(sockfd);
}

