#include "bitmap.h"
#include "font.h"
#include "message.h"
#include "lcd_info.h"
#include "client.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <math.h>
#include <sys/ioctl.h>
#include <time.h>
#include <pthread.h>

extern pthread_mutex_t mutex;	//msg区域锁，在client.c中定义



void *show_msg(char *msg, int x, int y, int width, int height, struct LcdDevice *lcd)
{
	
	//定义buf存放背景
	unsigned int buf[width*height];
	unsigned int *p = lcd->mp + y*lcd ->width + x;
	pthread_mutex_lock(&mutex);			//上锁
	memcpy(buf, p, width*height*4);
	pthread_mutex_unlock(&mutex);			//解锁

	// 打开字体
	font *f = fontLoad("/usr/share/fonts/DroidSansFallback.ttf");
	fontSetSize(f, 64);
	bitmap *bm;
	int i = 800;
	int msg_size = strlen(msg);
	int max = 0-(msg_size*17);
	while(1)
	{
		bm = createMyBitmap(width, height, 4, buf);
		fontPrint(f, bm, i, 10, msg, getColor(255, 255, 255, 255), 0);

		pthread_mutex_lock(&mutex);			//上锁
		for(int i=0; i<height; i++)
		{
			for(int j=0; j<width; j++)
			{
				memcpy(p+j, bm->map+j*4+i*width*4, 4);
			}
			p+=800;
		}
		pthread_mutex_unlock(&mutex);			//解锁
		usleep(10);
		i --;
		if (i < max)
		{
			i = 800;
		}
		//printf("i = %5d, max = %d\n", i, max);
		p = lcd->mp + y*lcd ->width + x;
		destroyBitmap(bm);
	}
	

	destroyBitmap(bm);
	fontUnload(f);
}

/**
 * 读取链表中的消息后销毁链表
 * @param  arg 传入NAMELIST链表
 * @return     空
 */
void *ad_msg(void *arg)
{
	struct LcdDevice* lcd = init_lcd("/dev/fb0");
	NAMELIST *head = (NAMELIST *)arg;
	char massage[2048] = {0};
	NAMELIST *pos;
	list_for_each_entry(pos, &(head ->list), list)
	{
		strcat(massage, pos ->name);
		strcat(massage, "        ");
	}

	show_msg(massage, 0, 400, 800, 80, lcd);
	destroy_list(head);
	destroy_lcd(lcd);
}


void *show_weather(char *msg, int x, int y, int width, int height, struct LcdDevice *lcd, unsigned int color, int font_x)
{
	// 打开字体
	font *f = fontLoad("/usr/share/fonts/DroidSansFallback.ttf");
	fontSetSize(f, 32);
	bitmap *bm;

	unsigned int *p = lcd->mp + y*lcd ->width + x;
		bm = createBitmapWithInit(width, height, 4, color);
		fontPrint(f, bm, font_x, 10, msg, getColor(255, 255, 255, 255), 0);

		for(int i=0; i<height; i++)
		{
			for(int j=0; j<width; j++)
			{
				memcpy(p+j, bm->map+j*4+i*width*4, 4);
			}
			p+=800;
		}

		//printf("i = %5d, max = %d\n", i, max);

	destroyBitmap(bm);
	fontUnload(f);
}
