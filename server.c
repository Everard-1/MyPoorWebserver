#include "server.h"
#include <arpa/inet.h>
#include <sys/epoll.h>

int initListenFd(unsigned short port)
{
	//1.创建监听的套接字
	int lfd = socket(AF_INET, SOCK_STREAM, 0);	//使用 IPv4 (AF_INET)，面向连接的 TCP 协议 (SOCK_STREAM)
	if (lfd == -1) {
		perror("socket");
		return -1;
	}
	//2.设置端口复用
	int opt = 1;	//表示启用
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));		//SOL_SOCKET 表示通用套接字选项，SO_REUSEADDR 允许端口复用
	if (ret == -1) {
		perror("setsockopt");
		return -1;
	}
	//3.绑定
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);	//设置端口号，使用 htons 将主机字节序转换为网络字节序
	addr.sin_addr.s_addr = INADDR_ANY;
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));		//将地址和端口绑定到套接字
	if (ret == -1) {
		perror("bind");
		return -1;
	}
	//4.设置监听
	ret = listen(lfd, 128);		//将套接字设置为被动监听模式，准备接受连接。第二个参数 128 指定了最大未决连接队列的长度
	if (ret == -1) {
		perror("listen");
		return -1;
	}
	//5.返回创建并绑定好的监听套接字文件描述符
	return lfd;
}

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
	while (1) {
		int num = epoll_wait(epfd, evs, size, -1);		//等待 epoll 事件，-1 表示无限等待
		for (int i = 0; i < num; i++) {
			int curfd = evs[i].data.fd;		//获取当前事件的文件描述符
			if (curfd == lfd) {
				//建立新连接
			}
			else {
				//通信->先接收数据，然后再回复数据
			}
		}
	}
	return 0;
}
