#include "client.h"
#include "lcd_info.h"
#include "cJSON.h"
#include "command.h"
#include "kernel_list.h"
#include "message.h"
#include "video.h"
#include "weather.h"
#include "beijing_time.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>			/* pthread   */
#include <sys/types.h>          /* socket    */
#include <sys/socket.h>			/* socket    */
#include <netinet/in.h>			/* inet_addr */
#include <arpa/inet.h>			/* htons     */
#include <sys/wait.h>


bool vid_change;
bool msg_change;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char const *argv[])
{
	//初始化lcd屏
	struct LcdDevice *lcd = init_lcd(FB0_PATH);
	if (lcd == NULL)
	{
		printf("init_lcd fail\n");
		//write_log
		return -1;
	}

	memset(lcd ->mp, 0xff, 800*480*4);
	memset(lcd ->mp + 400*lcd ->width, 0x00, 800*80*4);

	/**
	 * 连接服务端端
	 * sockfd记录连接服务器的端口
	 */
	int sockfd = connect_server();
	if (sockfd < 0)
	{
		printf("connect_server fail\n");
		return -1;
	}
	

	pthread_t msg_pid = 0;
	pthread_t vid_pid = 0;
	pthread_t wth_pid = 0;
	pthread_t tim_pid = 0;
	bool msg_init = false;
	bool vid_init = false;

	//创建线程显示天气
	char *city = "101010100";
	pthread_create(&wth_pid, NULL, weather, (void *)city);
	//创建线程显示时间
	beijing_time();	//校准北京时间
	pthread_create(&tim_pid, NULL, show_time, NULL);

	//创建管道文件用于控制mplayer退出
	//mkfifo("/fifo", 0777);

	//读取服务端消息
	vid_change = false;
	msg_change = false;
	char buf[JSON_BUF_SIZE] = {0};
	while(1)
	{
		memset(buf, 0, sizeof(buf));
		int ret = read(sockfd, buf, sizeof(buf));
		if (ret <= 0)
		{
			/**
			 * 客户端掉线
			 * 重新连接服务端
			 */
			//write_log
			printf("connect disable\n");
			sockfd = connect_server();
			continue;
		}

		/**
		 * 创建两个NAMELIST
		 * 用于保存视频列表和广告消息列表
		 */
		NAMELIST *vid_head = init_list();
		NAMELIST *msg_head = init_list();

		printf("recv: %s\n", buf);
		//读取信息成功
		//write_log("read success");
		ret = deal_json(buf, vid_head, msg_head);
		if (ret != 0)
		{
			//write_log("deal_json fail");
			printf("deal_json fail\n");
		}

		if (vid_change)
		{
			//检查文件是否存在
			//存在则写进playlist.lst
			//否则写进列表并创建线程进行下载
			update_list(vid_head);

			if (vid_init)
			{
				// system("killall mplayer");				//关闭mplayer进程
				// pthread_cancel(vid_pid);				//关闭线程
				pid_t pid = fork();
				if (pid < 0)
				{
					perror("fork fail");
				}
				if (pid == 0)
				{
					//mplayer -zoom -x 680 -y 400 -loop 0 -playlist  /massstorage/playlist.lst
					execlp("/bin/killall", "killall", "mplayer", NULL);
				}
				int status = 0;
				waitpid(pid, &status,0);
				
			}
			pid_t pid = fork();
			if (pid < 0)
			{
				perror("fork fail");
			}
			if (pid == 0)
			{
				//mplayer -zoom -x 680 -y 400 -loop 0 -playlist  /massstorage/playlist.lst
				execlp("/mplayer", "mplayer", "-zoom", "-x", "680", "-y", "400", "-loop", "0","-slave", "-playlist", "/massstorage/playlist.lst", NULL);
			}
			//pthread_create(&vid_pid, NULL, play_vid, (void *)vid_head);	//vid_head在线程函数中被销毁
			vid_change =  false;
			vid_init = true;
		}else { destroy_list(vid_head); }
		if (msg_change)
		{
			//消息列表有更新
			//创建线程显示消息
			if (msg_init)
			{
				printf("cancel pthread [%d]\n", msg_pid);
				pthread_cancel(msg_pid);
				pthread_mutex_lock(&mutex);				//上锁
				memset(lcd ->mp + 400*lcd ->width, 0x00, 800*80*4);
				pthread_mutex_unlock(&mutex);			//解锁
			}
			pthread_create(&msg_pid, NULL, ad_msg, (void *)msg_head);	//msg_list在线程函数中被销毁
			msg_change = false;
			msg_init = true;
		}else { destroy_list(msg_head); }
	}


	close(sockfd);
	destroy_lcd(lcd);
	return 0;
}

int connect_server(void)
{
	int sockfd = -1;

	while(1)
	{
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			//write_log("socket fail");
			perror("socket fail");
		}
		else	//创建socket成功
		{
			while(1)
			{
				//write_log("socket success");
				//创建服务器地址和地址结构体字节长度
				struct sockaddr_in sockaddr;
				sockaddr.sin_family = AF_INET;
				sockaddr.sin_port = htons(SERVER_PORT);
				sockaddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
				socklen_t socklen = sizeof(sockaddr);

				int ret = connect(sockfd, (struct sockaddr *)&sockaddr, socklen);
				if (ret < 0)
				{
					//write_log("connect server fail");
					perror("connect server fail");
					continue;
				}
				else
				{
					//write_log("connect server success");
					printf("connect server success\n");
					break;
				}
			}
			break;
		}
	}

	return sockfd;
}


/**
 * [deal_json description]
 * 提取json数据给analyze_json函数进行分析
 * @param  read_buf [description]
 * 从socket读取到的包含html头的char指针
 * @return          [description]
 */
int deal_json(char *read_buf, NAMELIST *vid_head, NAMELIST *msg_head)
{
	if (read_buf == NULL)
	{
		//write_log("deal_json: read_buf is NULL");
		return -1;
	}


	//如果有html头则跳过html头
	char *ptr = strstr(read_buf, "\r\n\r\n");
	if (ptr == NULL)
	{
		//write_log("deal_json: read_buf no html head");
		printf("deal_json: read_buf no html head\n");
		ptr = read_buf;
	}
	else
	{
		ptr += 4;
	}

	
	ptr = strstr(ptr, "{");		//查找json数据起始地址
	if (ptr == NULL)
	{
		//write_log("deal_json: read_buf no have json");
		printf("deal_json: read_buf no have json\n");
		return -1;
	}

	analyze_json(ptr, vid_head, msg_head);
	return 0;
}


void analyze_json(char *json, NAMELIST *vid_head, NAMELIST *msg_head)
{
	if (json == NULL)
	{
		//write_log("analyze_json: json is NULL");
		return ;
	}

	cJSON *root = cJSON_Parse(json);
	if (root == NULL)
	{
		//write_log("analyze_json: str is not json data");
		return ;
	}

	//读取system命令
	cJSON *sys_arr = cJSON_GetObjectItem(root, SYSTEM);
	int sys_count  = cJSON_GetArraySize(sys_arr);
	for (int i = 0; i < sys_count; ++i)
	{
		cJSON *tmp = cJSON_GetArrayItem(sys_arr, i);
		cJSON *obj = cJSON_GetObjectItem(tmp, NAME);
		printf("%s\n", obj ->valuestring);
		/* code here */

	}

	//读取video
	cJSON *vid_arr = cJSON_GetObjectItem(root, VIDEO);
	int vid_count  = cJSON_GetArraySize(vid_arr);
	for (int i = 0; i < vid_count; ++i)
	{
		cJSON *tmp = cJSON_GetArrayItem(vid_arr, i);
		cJSON *obj = cJSON_GetObjectItem(tmp, NAME);
		printf("%s\n", obj ->valuestring);
		/* code here */
		add_node(vid_head, obj ->valuestring);
		//在视频更新完后赋值为true
		vid_change = true;
	}


	//读取msg
	cJSON *msg_arr = cJSON_GetObjectItem(root, MSG);
	int msg_count  = cJSON_GetArraySize(msg_arr);
	for (int i = 0; i < msg_count; ++i)
	{
		printf("%d\n", msg_count);
		cJSON *tmp = cJSON_GetArrayItem(msg_arr, i);
		cJSON *obj = cJSON_GetObjectItem(tmp, NAME);
		printf("%s\n", obj ->valuestring);
		/* code here */
		add_node(msg_head, obj ->valuestring);
		msg_change = true;
	}


	cJSON_Delete(root);
}


NAMELIST *init_list()
{
	NAMELIST *head = (NAMELIST *)malloc(sizeof(NAMELIST));
	if (head == NULL) return NULL;

	head ->name = NULL;
	head ->bmpbuf = NULL;
	INIT_LIST_HEAD(&(head ->list));
	return head;
}

void destroy_list(NAMELIST *head)
{
	if (head == NULL) return;
	NAMELIST *pos = NULL;
	NAMELIST *n   = NULL;

	list_for_each_entry_safe(pos, n, &head ->list, list)
	{
		list_del(&(pos ->list));
		free(pos ->name);
		if (pos ->bmpbuf)
		{
			free(pos ->bmpbuf);
		}
		free(pos);
	}
}

int add_node(NAMELIST *head, char *name)
{
	if (head == NULL) return -1;
	if (name == NULL) return -1;

	NAMELIST *new_node = (NAMELIST *)malloc(sizeof(NAMELIST));
	new_node ->name = (char *)malloc(strlen(name)+1);
	memset(new_node ->name, 0, strlen(new_node ->name)+1);
	strcpy(new_node ->name, name);
	new_node ->bmpbuf = NULL;

	INIT_LIST_HEAD(&(new_node ->list));
	list_add_tail(&(new_node ->list), &(head ->list));

	return 0;
}