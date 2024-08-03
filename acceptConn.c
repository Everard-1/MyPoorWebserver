#include "server.h"
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>



int acceptConn(int lfd, int epfd)
{
	//1.建立新连接
	int cfd = accept(lfd, NULL, NULL);	//接受一个新的客户端连接，并返回新的客户端套接字文件描述符 cfd
	if (cfd == -1) {
		perror("accept");
		return -1;
	}
	//2.设置通信描述符为非阻塞
	int flag = fcntl(cfd, F_GETFL);		//获取当前文件描述符 cfd 的标志。
	flag |= O_NONBLOCK;		//将文件描述符设置为非阻塞模式
	fcntl(cfd, F_SETFL, flag);		//将修改后的标志设置回文件描述符 cfd
	//3.通信的文件描述符添加到epoll模型中
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;		//表示有数据可读且采用边沿触发模式
	ev.data.fd = cfd;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);		//将新的客户端文件描述符添加到 epoll 实例 epfd 中进行事件监控
	if (ret == -1) {
		perror("epoll_ctl");
		return -1;
	}
	return 0;
}
