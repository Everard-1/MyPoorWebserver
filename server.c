#include "server.h"
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>

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
	int flag = 0;
	while (1) {
		if (flag) {
			break;
		}
		int num = epoll_wait(epfd, evs, size, -1);		//等待 epoll 事件，-1 表示无限等待
		for (int i = 0; i < num; i++) {
			int curfd = evs[i].data.fd;		//获取当前事件的文件描述符
			if (curfd == lfd) {
				//建立新连接
				int ret = acceptConn(lfd, epfd);
				if (ret == -1) {
					//规定：建立连接失败，直接终止程序
					flag = 1;
					break;
				}
			}
			else {
				//通信->先接收数据，然后再回复数据
			}
		}
	}
	return 0;
}

int acceptConn(int lfd, int epfd)
{
	//1.建立新连接
	int cfd = accept(lfd, NULL, NULL);	//接受一个新的客户端连接，并返回新的客户端套接字文件描述符 cfd
	if (cfd == -1) {
		perror("accept");
		return -1;
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
}

int recvHttpRequest(int cfd)
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
		// 遍历字符串，党羽第一个\r\n的时候意味着请求行拿到了
		char* pt = strstr(buf, "\r\n");
		//计算请求行长度
		int reqlen = pt - buf;
		//保留请求行就可以
		buf[reqlen] = '\0';		//字符串截断
		//解析请求行

	}
	else if (len == 0) {
		printf("客户端断开了连接...\n");
		//服务器和客户端断开连接，文件描述符从epoll模型中删除
	}
	else {
		perror("recv");
		return -1;
	}
	return 0;
}

int parseRequestLine(const char* reqLine)
{
	//请求行分为三部分
	//GET /hello/world/ http/1.1
	//1.将请求行的三部分依次拆分，有用的前两部分
	// - 提交数据的方式
	// - 客户端向服务器请求的文件名

	//2.判断请求方式是不是get，不是get方式直接忽略

	//3.判断用户提交的请求是要访问服务器的文件还是目录
	//   /hello/world/
	//   - 第一个 / : 服务器提供的资源根目录，在服务器端可以随意制定
	//   - hello/world/ -> 服务器资源根目录中的两个目录
	//需要在程序中判断得到的文件的属性 - stat()

	//4.客户端请求的名字是一个文件，发送文件内容给客户端
	
	//5.客户端请求的名字是一个目录，遍历目录，发送目录内容给客户端
	return 0;
}
