#ifndef LCD_INFO_H
#define LCD_INFO_H

#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "point.h"

//bmp图片头
#pragma pack(1) //1字节对齐分配空间

typedef struct BmpHeader
{
	short bfType;//文件类型2字节 "BM"
	int bfSize;//文件大小
	short bfReserved1;  //保留为0
	short bfReserved2;  //保留为0
	int bfOffBits;     //文件头到实际像素的偏移量（文件头+图像信息头大小）

	int biSize ;
	int biWidth;
	int biHeight;
	short biPlanes;
	short biBitCount;
	int biCompression;
	int biSizeImage;
	int biX;
	int biY;
	int biClrused;
	int biClrImportant;
}BmpHeader;

#pragma pack(0)

//lcd设备
struct LcdDevice
{
	int fd;
	int width;
	int height;
	int pixByte;

	unsigned int *mp;	//映射首地址
	unsigned int color; //默认颜色
};

//点结构体

//矩形结构体

//初始化Lcd
struct LcdDevice *init_lcd(const char *device);

//销毁Lcd
bool destroy_lcd(struct LcdDevice *lcd);

//设置默认绘制颜色
void set_color(struct LcdDevice *lcd, unsigned int color);


void draw_point(struct LcdDevice* lcd, int x, int y);

//填充矩形
#define draw_rect(lcd, rect) draw_rect_c(lcd, rect, lcd ->color)
void draw_rect_c(struct LcdDevice *lcd, struct Rect rect, unsigned int color);

//绘制矩形
#define draw_rect_edge(lcd, rect) draw_rect_c(lcd, rect, lcd ->color)
void draw_rect_edge_c(struct LcdDevice *lcd, struct Rect rect, unsigned int color);

//绘制圆形
void draw_circle(struct LcdDevice *lcd, int x, int y, int r);

//绘图
void drow_bmp(struct LcdDevice *lcd, const char *picpath,struct Rect rect);

//无缩放绘图
void draw_bmp_no_zoom(struct LcdDevice* lcd, int x, int y, const char *picPath);


#endif	