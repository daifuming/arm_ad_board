#ifndef TIMING_H_
#define TIMING_H_


struct LcdDevice *init_lcd_timing(const char *device);
void * start_timing(void *arg);
void *show_msg(char *msg, int x, int y, int width, int height, struct LcdDevice *lcd);


/**
 * 显示消息到lcd一定区域
 * @param  msg    显示的消息
 * @param  x      显示起点x
 * @param  y      显示起点y
 * @param  width  显示区域宽度
 * @param  height 显示区域高度
 * @param  lcd    显示的lcd屏
 * @param  color  显示的背景颜色
 * @return        NULL
 */
void *show_weather(char *msg, int x, int y, int width, int height, struct LcdDevice *lcd, unsigned int color, int font_x);

/**
 * [轮播msg线程函数]
 * @param  arg [description]
 * @return     [description]
 */	
void *ad_msg(void *arg);


#endif