#include "cJSON.h"
#include "lcd_info.h"
#include "message.h"
#include "beijing_time.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <time.h>


//http://quan.suning.com/getSysTime.do
#define HTTP_REQ "GET %s HTTP/1.1\r\nHost:%s\r\nConnection: close\r\n\r\n"

static void json_parse(char *json, char *time_info)
{
	cJSON *root = cJSON_Parse(json);
	cJSON *time_obj = cJSON_GetObjectItem(root, "sysTime2");

	strcpy(time_info, time_obj ->valuestring);
	cJSON_Delete(root);
}



void *beijing_time()
{
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
	addr.sin_addr.s_addr = inet_addr("125.90.204.170");
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
	sprintf(request,HTTP_REQ,"/getSysTime.do","quan.suning.com");

	//发送请求，接收数据并分析
	ret = write(sockfd, request, strlen(request));
	if (ret <= 0)
	{	perror("send error"); }
	char buf[1024] = {0};
	read(sockfd, buf, sizeof(buf));
	char *ptr = strstr(buf, "\r\n\r\n") + 4;
	ptr = strstr(ptr, "{");

	char time_info[64];
	json_parse(ptr, time_info);

	printf("time: %s\n", time_info);
	//set_systime(time_info);
	char cmd[32] = {0};
	sprintf(cmd, "date -s \"%s\"", time_info);
	system(cmd);
	close(sockfd);
}


void *show_time(void *arg)
{
	struct LcdDevice* lcd = init_lcd("/dev/fb0");

	while(1)
	{
		//从系统获取时间
		time_t t;
		time(&t);

		struct tm *mtm = gmtime(&t);
		char date[32] = {0};
		char time[32] = {0};
		sprintf(date, "%04d-%02d-%02d", 1900+mtm->tm_year,mtm->tm_mon+1,mtm->tm_mday);
		sprintf(time, "%02d:%02d:%02d",  mtm->tm_hour,mtm->tm_min,mtm->tm_sec);
				
		//(char *msg, int x, int y, int width, int height, struct LcdDevice *lcd, unsigned int color, font_x)
		show_weather(date, 680, 300,  120, 50, lcd, 0x00ffffff, 0);
		show_weather(time, 680, 350,  120, 50, lcd, 0xff000000, 15);

		sleep(1);
	}

	destroy_lcd(lcd);
}