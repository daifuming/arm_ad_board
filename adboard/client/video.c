#include "client.h"
#include "kernel_list.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>





//mplayer -zoom -x 680 -y 400 /massstorage/video1.avi 
//mplayer -zoom -x 680 -y 400  -input file=/fifo /massstorage/video1.avi 


void *play_vid(void *arg)
{
	NAMELIST *head = (NAMELIST *)arg;
	NAMELIST *pos;
	int size = 0;
	list_for_each_entry(pos, &(head ->list), list)
	{
		//获取视频条数
		size ++;
		//system("mplayer -zoom -x 680 -y 400  -input file=/fifo /massstorage/video1.avi");
	}
	char buf[size][1024];
	memset(buf, 0, sizeof(buf));
	int i = 0;
	list_for_each_entry(pos, &(head ->list), list)
	{
		//获取视频路径
		strcpy(buf[i], pos ->name);
		i ++;
	}
	destroy_list(head);	//销毁链表
	for (int j = 0; j < size; ++j)
	{
		char cmd[1024] = {0};
		sprintf(cmd, "mplayer -zoom -x 680 -y 400 -quiet -input file=/fifo %s", buf[j]);
		system(cmd);
	}
}