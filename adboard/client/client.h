#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>
#include "kernel_list.h"

#define FB0_PATH "/dev/fb0"

#define SERVER_ADDR "192.168.110.177" 	/* 服务端地址 */
#define SERVER_PORT 6789				/* 服务端端口 */

#define JSON_BUF_SIZE 1024				/* 保存json数据的buf大小 */

typedef struct
{
	char *name;
	unsigned int *bmpbuf;
	struct list_head list;
}NAMELIST;


/**
 * 连接服务端
 * 成功返回连接的socket文件描述符
 */
int connect_server(void);

/**
 * [deal_json description]
 * 分析从socket读到的字符串，提取json数据给analyze_json处理
 * @param  read_buf [description]
 * 从socket读到的字符串
 * @return          [description]
 */
int deal_json(char *read_buf, NAMELIST *vid_list, NAMELIST *msg_list);

/**
 * [analyze_json description]
 * 接收json字符串进行解析
 * @param json [description]
 * 一个有效的json字符串
 */
void analyze_json(char *json, NAMELIST *vid_list, NAMELIST *msg_list);

/**
 * [init_list description]
 * 初始化一个内核链表，头节点name=NULL
 * @return [description]
 * 返回一个已经初始化的只有一个头节点的链表
 */
NAMELIST *init_list();

/**
 * [destroy_list description]
 * 销毁一个链表，释放链表占用的对内存
 * @param head [description]
 * 需要销毁的链表头节点指针
 */
void destroy_list(NAMELIST *head);


/**
 * [add_node description]
 * 添加一个节点到一个链表中
 * @param  head [需要添加的目的链表]
 * @param  name [新节点中name要填充的内容]
 * @return      [成功返回0]
 */
int add_node(NAMELIST *head, char *name);

#endif