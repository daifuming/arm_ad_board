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


//初始化Lcd
struct LcdDevice *init_lcd_timing(const char *device)
{
	struct LcdDevice* lcd = malloc(sizeof(struct LcdDevice));
	if(lcd == NULL) return NULL;
	//1打开设备
	lcd->fd = open(device, O_RDWR);
	if(lcd->fd < 0)
	{
		perror("open lcd fail");
		free(lcd);
		return NULL;
	}

	//2.获取lcd设备信息
	struct fb_var_screeninfo info; //存储lcd信息结构体--在/usr/inlucde/linux/fb.h文件中定义
	int ret = ioctl(lcd->fd, FBIOGET_VSCREENINFO, &info);
	if(ret < 0)
	{
		perror("request fail");
	}

	lcd->width = info.xres;
	lcd->height = info.yres;
	lcd->pixByte = info.bits_per_pixel/8;//每一个像素占用的字节数
	
	//映射
	lcd->mp = mmap(NULL, lcd->width*lcd->height*lcd->pixByte, 
				    PROT_READ|PROT_WRITE,MAP_SHARED, lcd->fd, 0);
	if(lcd->mp == (void *)-1)
	{
		perror("mmap fail");		
	}
	//给lcd设置默认颜色
	lcd->color = 0x000ff00f;
	
	return lcd;
}


// //首页的时钟背景颜色为#38312e
// void *start_timing(void *arg)
// {
	
// 	struct LcdDevice* lcd = (struct LcdDevice*)arg;	
// 	// 打开字体
// 	font *f = fontLoad("/usr/share/fonts/DroidSansFallback.ttf");
// 	fontSetSize(f, 28);
// 	//memset(lcd ->mp, 0x00, 800*480*4);
// 	// 创建bitmap
// 	time_t tm, tm_new;
// 	time(&tm);
// 	int scn;
// 	char buf[128] = {0};

// 	while(!flag_game_end)
// 	{
// 		bitmap *bm = createBitmap(240, 36, 4);
// 		time(&tm_new);
// 		scn = tm_new - tm;
// 		memset(buf, 0, sizeof(buf));
// 		sprintf(buf, "本关用时%d秒", scn);
// 		fontPrint(f, bm, 20, 10, buf, getColor(255, 255, 0,0), 0);

// 		unsigned int *p = lcd->mp + 410*lcd ->width + 55;
// 		for(int i=0; i<36; i++)
// 		{
// 			for(int j=0; j<240; j++)
// 			{
// 				memcpy(p+j, bm->map+j*4+i*240*4, 4);
// 			}
// 			p+=800;
// 		}
// 		destroyBitmap(bm);
// 		sleep(1);
// 	}
	
// 	// 关闭字体
// 	fontUnload(f);
// 	// 关闭bitmap
	
// }

/*
void *show_msg(char *msg, int x, int y, int width, int height)
{
	
	struct LcdDevice* lcd = init_lcd("/dev/fb0");	
	// 打开字体
	font *f = fontLoad("/usr/share/fonts/DroidSansFallback.ttf");
	fontSetSize(f, 28);
	//memset(lcd ->mp, 0x00, 800*480*4);
	// 创建bitmap


		bitmap *bm = createBitmap(240, 36, 4);
		fontPrint(f, bm, 20, 10, msg, getColor(255, 255, 0,0), 0);

		unsigned int *p = lcd->mp + 410*lcd ->width + 55;
		for(int i=0; i<36; i++)
		{
			for(int j=0; j<240; j++)
			{
				memcpy(p+j, bm->map+j*4+i*240*4, 4);
			}
			p+=800;
		}
		destroyBitmap(bm);
		sleep(1);

	// 关闭字体
	fontUnload(f);
	// 关闭bitmap
}
*/


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
}