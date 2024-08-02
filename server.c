#include "server.h"
#include <arpa/inet.h>

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

int epollRun()
{
	//1.创建epoll模型
	epoll_create();
	epoll_ctl();
	//检测

	return 0;
}
