#include "server.h"
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/stat.h>
#include <strings.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>

int recvHttpRequest(int cfd,int epfd)
{
	//临时缓冲区用于存储从套接字读取的数据
	char tmp[1024];		//每次接收1k数据
	char buf[4096];		//将每次读到的数据存储到这个buf中

	//循环读数据
	int len, total = 0;	//total：当前buf中已经存储了多少数据

	//没有必要将所有的http请求全部保存下来
	//因为需要的数据都在请求行中
	//-客户端向服务器请求的都是静态资源，请求的资源内容在请求行的第二部分
	//-只需要将请求完整的保存下来就可以，请求行后边请求头和空行
	//-不需要解析请求头中的数据，因此接收到 之后不存储也是没问题的
	while ((len = recv(cfd, tmp, sizeof(tmp), 0)) > 0){			//使用 recv 函数读取数据到 tmp 缓冲区
		if (total + len < sizeof(buf)) {
			//有空间存数据
			memcpy(buf + total, tmp, len);
		}
		total += len;	//更新total
	}

	//循环结束--->读完了
	//读操作是非阻塞的，当前缓存中没有数据值返回-1，errno==EAGAIN
	if (len == -1 && errno == EAGAIN) {
		//将请求行从接受的数据中拿出来
		//在http中换行使用的是\r\n
		// 遍历字符串，当遇到第一个\r\n的时候意味着请求行拿到了
		char* pt = strstr(buf, "\r\n");
		//计算请求行长度
		int reqlen = pt - buf;
		//保留请求行就可以
		buf[reqlen] = '\0';		//字符串截断
		//解析请求行
		parseRequestLine(cfd,buf);
	}
	else if (len == 0) {
		printf("客户端断开了连接...\n");
		//服务器和客户端断开连接，文件描述符从epoll模型中删除
		disConnect(cfd, epfd);
	}
	else {
		perror("recv");
		return -1;
	}
	return 0;
}
