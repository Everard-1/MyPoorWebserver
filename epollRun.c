#include "server.h"
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>

int epollRun(unsigned short port)
{
	//1.创建epoll模型
	int epfd = epoll_create(10);	//参数 10 是提示内核需要多少个文件描述符来完成操作，实际现在已经被忽略了，只需要大于0即可
	if (epfd == -1) {
		perror("epoll_create");
		return -1;
	}
	//2.初始化epoll模型
	int lfd = initListenFd(port);	//初始化一个监听套接字，绑定到指定端口并开始监听连接
	struct epoll_event ev;		//定义一个 epoll 事件结构体
	ev.events = EPOLLIN;	//设置事件类型为 EPOLLIN，表示有数据可读
	ev.data.fd = lfd;		//设置事件的文件描述符为监听套接字的文件描述符 lfd
	//添加lfd 到检测模型中
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);		//将监听套接字 lfd 添加到 epoll 实例 epfd 中，并关注其 EPOLLIN 事件。
	if (epfd == -1) {
		perror("epoll_ctl");
		return -1;
	}
	//检测 - 循环检测
	struct epoll_event evs[1024];		//定义一个数组 evs，用于存储 epoll 返回的事件
	int size = sizeof(evs) / sizeof(evs[0]);
	int flag = 0;
	while (1) {
		if (flag) {
			break;
		}
		//主线程不停的调用epoll_wait
		int num = epoll_wait(epfd, evs, size, -1);		//等待 epoll 事件，-1 表示无限等待
		for (int i = 0; i < num; i++) {
			int curfd = evs[i].data.fd;		//获取当前事件的文件描述符
			if (curfd == lfd) {
				//建立新连接
				//创建子线程,在子线程中建立新的连接
				//acceptConn是子线程的回调
				int ret = acceptConn(lfd, epfd);
				if (ret == -1) {
					//规定：建立连接失败，直接终止程序
					flag = 1;
					break;
				}
			}
			else {
				//通信->先接收数据，然后再回复数据
				//创建线程，在子线程中通信，recvHttpRequest也是子线程的回调
				recvHttpRequest(curfd, epfd);
			}
		}
	}
	return 0;
}
