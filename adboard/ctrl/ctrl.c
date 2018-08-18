#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "cJSON.h"

#define BUF_SIZE 1024
#define IP_PORT_SIZE 24		//存放ip+port的数组大小


int main(int argc, char const *argv[])
{
	printf("initing...\n");
	//创建socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror("client: socket sockfd fail");
		return -1;
	}
	//连接服务端
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(struct sockaddr_in));	//IP地址
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(6789);
	sockaddr.sin_addr.s_addr = inet_addr("192.168.110.177");

	int ret = connect(sockfd, (struct sockaddr *)&sockaddr, sizeof(struct sockaddr));
	if (ret < 0)
	{
		perror("client: connect fail");
		close(sockfd);
		return -1;
	}
	printf("done...\n");

	

	//发送数据
	int op = 0;
	char dev_ip[20][IP_PORT_SIZE] = {0};

	/**
	 * 创建cJSON对象
	 */
	 cJSON *root = cJSON_CreateObject();
	 cJSON *sys_array =  cJSON_CreateArray();
	 cJSON *vid_array = cJSON_CreateArray();
	 cJSON *msg_array = cJSON_CreateArray();

	while(1)
	{
		
		system("clear");
		printf("1:syscmd;\n2:add video name;\n3:add message;\n4:done\nselect op:");
		scanf("%d", &op);
		if (op == 1)
		{
			cJSON *cmd_obj = cJSON_CreateObject();;
			printf("input system command:");
			char cmd_buf[128] = {0};
			scanf("%s", cmd_buf);
			cJSON_AddStringToObject(cmd_obj, "7", cmd_buf);
			cJSON_AddItemToArray(sys_array, cmd_obj);
		}
		else if(op == 2)
		{
			cJSON *vid_obj = cJSON_CreateObject();;
			printf("input vid_name:");
			char vid_buf[128] = {0};
			scanf("%s", vid_buf);
			cJSON_AddStringToObject(vid_obj, "7", vid_buf);
			cJSON_AddItemToArray(vid_array, vid_obj);
		}
		else if (op == 3)
		{
			cJSON *msg_obj = cJSON_CreateObject();;
			printf("input msg_name:");
			char msg_buf[128] = {0};
			scanf("%s", msg_buf);
			cJSON_AddStringToObject(msg_obj, "7", msg_buf);
			cJSON_AddItemToArray(msg_array, msg_obj);
		}
		else if (op == 4)
		{
			cJSON_AddItemToObject(root, "1", sys_array);
			cJSON_AddItemToObject(root, "2", vid_array);
			cJSON_AddItemToObject(root, "3", msg_array);
			char *json = cJSON_Print(root);
			printf("%s\n", json);
			write(sockfd, json, strlen(json));
			cJSON_Delete(root);
			root = cJSON_CreateObject();
			sys_array =  cJSON_CreateArray();
			vid_array = cJSON_CreateArray();
			msg_array = cJSON_CreateArray();
		}
		else
		{
			printf("input a available number!\n");
			getchar();
		}
	}
	

	/**
	 * 关闭客户端
	 */
	close(sockfd);
	return 0;
}