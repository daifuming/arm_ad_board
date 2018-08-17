#ifndef TIMING_H_
#define TIMING_H_


struct LcdDevice *init_lcd_timing(const char *device);
void * start_timing(void *arg);
void *show_msg(char *msg, int x, int y, int width, int height, struct LcdDevice *lcd);

/**
 * [轮播msg线程函数]
 * @param  arg [description]
 * @return     [description]
 */	
void *ad_msg(void *arg);

#endif