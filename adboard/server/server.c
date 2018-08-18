#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <stdbool.h>
#include <stdlib.h>

#include "kernel_list.h"

#define BUFSIZE 1024			//接收数据缓冲区大小
#define FD_SIZE 100			//客户端最大连接数99
#define IP_PORT_SIZE 24		//存放ip+port的数组大小

#define DEV_BOARD "GEC6818"	//开发板主机名


/**
 * 连接上的开发板链表
 */
struct BoardNode
{
	int fd;					//socket文件描述符
	bool flag;				//设备类型,true为开发板
	char name[8];			//设备名称
	char ip[16];			//设备ip
	int port;				//设备端口号
	struct list_head list;	//内核链表结构
};

int main(int argc, char const *argv[])
{
	//创建套接字
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror("server: socket sockfd fail");
		return -1;
	}

	/**
	 * 创建链表头
	 * 用于保存当前连接的开发板信息
	 */

	//获取地址
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(6789);
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//绑定端口
	int ret = bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(struct sockaddr));
	if (ret < 0)
	{
		perror("server: bind port fail");
		close(sockfd);
		return -1;
	}

	//监听端口
	ret = listen(sockfd, 5);
	if (ret < 0)
	{
		perror("server: listen port fail");
		close(sockfd);
		return -1;
	}

	
	 /**
	  * 创建epoll句柄
	  * int epoll_create(int size);
	  * size为需要监视的数目
	  * 返回一个epoll句柄，占用一个文件描述符
	  * 使用完后需要关闭epfd文件描述符
	  */
	int epfd = epoll_create(20);

	/**
	 * 需要监听的事件类型
	 * struct epoll_event {
	 *	  __uint32_t events;  /* Epoll events 
	 *	  epoll_data_t data;  /* User data variable 
	 *	};
	 *	events:
	 *	EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
	 *	EPOLLOUT：表示对应的文件描述符可以写；
	 *	EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
	 *	EPOLLERR：表示对应的文件描述符发生错误；
	 *	EPOLLHUP：表示对应的文件描述符被挂断；
	 *	EPOLLET： 将EPOLL设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)来说的。
	 *	EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里
	 */
	struct epoll_event epevt;
	epevt.events = EPOLLIN|EPOLLET;
	epevt.data.fd = sockfd;

	/**
	 * 注册服务端sockfd到epoll中
	 * int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
	 * op:
	 * EPOLL_CTL_ADD：注册新的fd到epfd中；
	 * EPOLL_CTL_MOD：修改已经注册的fd的监听事件；
	 * EPOLL_CTL_DEL：从epfd中删除一个fd；
	 */
	epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &epevt);
	//evts用于记录发生的事件
	struct epoll_event evts[20];
	/**
	 * 创建链表用于保存连接设备信息
	 * 把sockfd放在头节点
	 */
	struct BoardNode *head = (struct BoardNode *)malloc(sizeof(struct BoardNode));
	if (head == NULL)
	{
		printf("malloc head fail\n");
	}
	else
	{
		head ->fd = sockfd;
		strcpy(head ->name, "server");
		strcpy(head ->ip, inet_ntoa(sockaddr.sin_addr));
		INIT_LIST_HEAD(&(head ->list));
	}



	while(1)
	{
		/**
		 * int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout);
		 * events:从内核得到事件的集合（数组）
		 * maxevents：events数组有多大，不能大于epoll_create时的size
		 * timeout:超时时间，毫秒，-1时表示永久阻塞
		 * 返回数值为发生的事件数量
		 */
		int ret = epoll_wait(epfd, evts, 20, -1);
		if (ret < 0)
		{
			perror("epoll_wait fail");
			continue;
		}

		//轮询发生的事件进行处理
		for (int i = 0; i < ret; ++i)
		{
			if(evts[i].data.fd == sockfd)
			{
				/**
				 * 客户端请求连接
				 * 创建sockaddr保存连接的客户端地址
				 */
				struct sockaddr_in client_addr;
				memset(&client_addr, 0, sizeof(struct sockaddr_in));
				socklen_t client_len = sizeof(client_addr);
				int conn_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
				if (conn_fd < 0)
				{
					//接收失败则进行下一个事件的处理
					perror("accept fail");
					continue;
				}
				else
				{
					/**
					 * 建立连接成功
					 * 把客户端socket描述符注册到epoll和链表中
					 */
					printf("accept success:");
					struct epoll_event epevt;
					epevt.events = EPOLLIN;
					epevt.data.fd = conn_fd;
					epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &epevt);
					//加到链表中
					struct BoardNode *new_node = (struct BoardNode *)malloc(sizeof(struct BoardNode));
					if (new_node == NULL)
					{
						printf("malloc new_node fail\n");
					}
					else
					{
						new_node ->fd = conn_fd;
						strcpy(new_node ->ip, inet_ntoa(client_addr.sin_addr));
						//printf("port is: %05d\n", (int) ntohs(client_addr.sin_port));
						new_node ->port = (int) ntohs(client_addr.sin_port);
						printf("%s\n", new_node ->ip);
						INIT_LIST_HEAD(&(new_node ->list));
						list_add_tail(&(new_node ->list), &(head ->list));
					}
				}
			}
			else	//其他建立的连接可读
			{
				char buf[BUFSIZE] = {0};
				int ret = read(evts[i].data.fd, buf, BUFSIZE);
				if (ret <= 0)
				{
					/**
					 * 客户端断开链接
					 * 把相应的socket文件描述符从epoll和链表中剔除
					 */
					printf("客户端掉线:");
					close(evts[i].data.fd);
					struct BoardNode *pos = NULL;
					struct BoardNode *n = NULL;
					list_for_each_entry_safe(pos, n, &head ->list, list)
					{
						if (pos ->fd == evts[i].data.fd)
						{
							printf("%s\n", pos ->ip);
							list_del(&(pos ->list));
							free(pos);
							break;
						}
					}
					epoll_ctl(epfd, EPOLL_CTL_DEL, evts[i].data.fd, &epevt);
				}
				else
				{
					struct BoardNode *pos = NULL;
					list_for_each_entry(pos, &(head ->list), list)
						if (pos ->fd != evts[i].data.fd)			//转发给出了控制端外的socket
							write(pos ->fd, buf, sizeof(buf));
				}
			}
		}
	}

	close(epfd);
	close(sockfd);
	return 0;
}