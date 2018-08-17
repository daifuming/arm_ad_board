#include "lcd_info.h"
#include <linux/fb.h>	//获取设备信息
#include <sys/ioctl.h>	//ioctl
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>



//初始化Lcd
struct LcdDevice *init_lcd(const char *device)
{
	struct LcdDevice *lcd = (struct LcdDevice *)malloc(sizeof(struct LcdDevice));
	if (lcd == NULL) return NULL;
	lcd ->fd = open(device, O_RDWR);
	if (lcd ->fd < 0)
	{
		perror("open fd fail");
		free(lcd);
		return NULL;
	}

	//get info 
	struct fb_var_screeninfo info;
	int ret = ioctl(lcd ->fd, FBIOGET_VSCREENINFO, &info);
	if (ret < 0) perror("ioctl fail");

	lcd ->width = info.xres;
	lcd ->height = info.yres;
	lcd ->pixByte = info.bits_per_pixel / 8;

	//mmap
	//				首地址，大小									
	lcd ->mp = mmap(NULL, lcd ->width*lcd ->width*lcd ->pixByte, 
					PROT_READ|PROT_WRITE , MAP_SHARED, lcd ->fd, 0);
	if (lcd ->mp == MAP_FAILED)	perror("mmap fail");

	//set color
	lcd ->color = 0x0000ff00;

	return lcd;
}

//销毁Lcd
bool destroy_lcd(struct LcdDevice *lcd)
{
	if(lcd == NULL) return false;
	int ret = munmap(lcd ->mp, lcd ->width*lcd ->width*lcd ->pixByte);
	if (ret == -1)	perror("munmap fail");
	free(lcd);
	close(lcd ->fd);
	return true;
}

//设置默认绘制颜色
void set_color(struct LcdDevice *lcd, unsigned int color)
{
	if (lcd == NULL)	return;
	lcd ->color = color;
}

inline void draw_point(struct LcdDevice* lcd, int x, int y)
{
	lcd->mp[x+y*lcd->width] = lcd->color;
}

//填充矩形
void draw_rect_c(struct LcdDevice *lcd, struct Rect rect, unsigned int color)
{
	if (lcd == NULL) return;
	if (rect.pointA.x < 0)	rect.pointA.x = 0;
	if (rect.pointA.y < 0)	rect.pointA.y = 0;
	if (rect.pointB.x < 0)	rect.pointB.x = 0;
	if (rect.pointB.y < 0)	rect.pointB.y = 0;
	if (rect.pointA.x > 800)	rect.pointA.x = 800;
	if (rect.pointA.y > 480)	rect.pointA.y = 480;
	if (rect.pointB.x > 800)	rect.pointB.x = 800;
	if (rect.pointB.y > 480)	rect.pointB.y = 480;
	unsigned int *p = lcd ->mp;
	p = p + rect.pointA.y * lcd ->width + rect.pointA.x;
	for (int i = 0; i < rect.pointB.y - rect.pointA.y; ++i)
	{
		for (int j = 0; j < rect.pointB.x - rect.pointA.x; ++j)
		{
			p[j] = color;
		}
		p += lcd ->width;
	}
}

void draw_rect_edge_c(struct LcdDevice *lcd, struct Rect rect, unsigned int color)
{
	if (lcd == NULL) return;
	if (rect.pointA.x < 0)	rect.pointA.x = 0;
	if (rect.pointA.y < 0)	rect.pointA.y = 0;
	if (rect.pointB.x < 0)	rect.pointB.x = 0;
	if (rect.pointB.y < 0)	rect.pointB.y = 0;
	if (rect.pointA.x > 800)	rect.pointA.x = 800;
	if (rect.pointA.y > 480)	rect.pointA.y = 480;
	if (rect.pointB.x > 800)	rect.pointB.x = 800;
	if (rect.pointB.y > 480)	rect.pointB.y = 480;
	unsigned int *p = lcd ->mp;
	p = lcd ->mp + (rect.pointA.y-1) * lcd ->width + rect.pointA.x;
	for (int i = 0; i < rect.pointB.x - rect.pointA.x; ++i)
	{
		p[i] = color;
		p[(rect.pointB.y - rect.pointA.y)*lcd ->width + i] = color;
	}
	p = lcd ->mp + rect.pointA.y * lcd ->width + rect.pointA.y;
	for (int i = 0; i < rect.pointB.y - rect.pointA.y; ++i)
	{
		p[0] = color;
		p[rect.pointB.x - rect.pointA.x] = color;
		p += lcd ->width;
	}

}

//绘制圆形
void draw_circle(struct LcdDevice *lcd, int x, int y, int r)
{
	if(r == 0)return;
    int a, b, num;
    a = 0;
    b = r;
    while(2 * b * b >= r * r)          
    {
        draw_point(lcd, x + a, y - b); // 0~1
        draw_point(lcd, x - a, y - b); // 0~7
        draw_point(lcd, x - a, y + b); // 4~5
        draw_point(lcd, x + a, y + b); // 4~3
        draw_point(lcd, x + b, y + a); // 2~3
        draw_point(lcd, x + b, y - a); // 2~1
        draw_point(lcd, x - b, y - a); // 6~7
        draw_point(lcd, x - b, y + a); // 6~5
        
        a++;
        num = (a * a + b * b) - r*r;
        if(num > 0)
        {
            b--;
            a--;
        }
    }
	draw_circle(lcd, x, y, r-1);
}


//绘图
void drow_bmp(struct LcdDevice *lcd, const char *picpath, struct Rect rect)
{
	FILE *fp = fopen(picpath, "r");
	if(fp == NULL)
	{
		perror("open pic fail");
		return ;
	} 

	BmpHeader bmpheader;
	fread(&bmpheader, sizeof(BmpHeader), 1, fp);

	long dpixeladd = bmpheader.biBitCount / 8;
	long LineByteWidth = bmpheader.biWidth * (dpixeladd);
	if ((LineByteWidth % 4) != 0)
	{
		printf("%ld\n", LineByteWidth);
		LineByteWidth += 4 - (LineByteWidth % 4);
		printf("%ld\n", LineByteWidth);
	}
	

	//printf("%d\n", bmpheader.biBitCount);
	unsigned char *rgb_buf_char = (unsigned char *)malloc(LineByteWidth*bmpheader.biHeight);
	//printf("rgb_buf_char = %d\n", bmpheader.biWidth*bmpheader.biHeight*3);
	if (rgb_buf_char == NULL)
	{
		printf("rgb_buf_char malloc fail\n");
		return;
	}
	fread(rgb_buf_char, LineByteWidth*bmpheader.biHeight, 1, fp);
	fclose(fp);

	//源的大小
	int ws = 0, hs = 0;
	ws = bmpheader.biWidth;
	hs = bmpheader.biHeight;
	//printf("ws = %d, hs = %d\n", ws, hs);
	//目标的大小
	int wd = 0, hd = 0;
	wd = rect.pointB.x - rect.pointA.x;
	hd = rect.pointB.y - rect.pointA.y;
	//printf("wd = %d, hd = %d\n", wd, hd);

	//记录目标的起始地址
	//printf("test\n");
	unsigned int *p = lcd ->mp + (rect.pointA.y) * lcd ->width + rect.pointA.x;
	//printf("%p %p\n", p, lcd ->mp);
	//unsigned char *q_tail = rgb_buf_char + bmpheader.biWidth*(bmpheader.biHeight-1)*3;
	unsigned char *q = rgb_buf_char;


	int index_x = 0, index_y = 0;
	for (int y = hd -1; y >= 0; --y)
	{
		for (int x = 0; x < wd; ++x)
		{
			index_x = (int)((((float)x)/wd)*ws);
			//printf("%d \n", index);
			p[x] = q[index_x*3] | q[index_x*3+1] << 8 | q[index_x*3+2] << 16;
		}
		p += lcd ->width;

		index_y = (int)((((float)y)/hd)*hs);
		q = rgb_buf_char + index_y*LineByteWidth;
	}

	free(rgb_buf_char);
}

//绘制bmp图片
void draw_bmp_no_zoom(struct LcdDevice* lcd, int x, int y, const char *picPath)
{
	//1.打开图片 
	FILE  *file =fopen(picPath, "r");
	if(file == NULL) perror("open bmp fail");
	
	//2.图片头+图片数据
	BmpHeader mBmpHeader;
	fread(&mBmpHeader, sizeof(BmpHeader), 1, file);
	
	int w = mBmpHeader.biWidth; //保存图片的宽和高
	int h = mBmpHeader.biHeight;
	
	//定义一个空间存储bmp图片像素
	unsigned char rgbBuf[mBmpHeader.bfSize - 54];
	fread(rgbBuf, sizeof(rgbBuf), 1, file);
	fclose(file);
	
	
	int dw = w+x > lcd->width? lcd->width-x : w;       
	int dh = h+y > lcd->height? lcd->height-y : h;
	
	
	unsigned int *p = lcd->mp + (y+dh)*lcd->width + x;
	unsigned char *trgb = rgbBuf;
	
	unsigned int color = 0;
	int i=0; 
	int j=0;
	for(i=0; i<dh; i++)
	{
		for(j=0; j<dw; j++)
		{
			memcpy(&color, trgb, 3);
			p[j] = color;
			trgb += 3;
		}
		p -= lcd->width;
		trgb += (w-dw)*3;
	}
}

