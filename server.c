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
				recvHttpRequest(curfd, epfd);
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

int parseRequestLine(int cfd, const char* reqLine)
{
	//请求行分为三部分
	//GET /hello/world/ http/1.1
	//1.将请求行的三部分依次拆分，有用的前两部分
	// - 提交数据的方式
	// - 客户端向服务器请求的文件名
	char method[6];
	char path[1024];
	sscanf(reqLine, "%[^ ] %[^ ]", method, path);

	//2.判断请求方式是不是get，不是get方式直接忽略
	//http中不区分大小写 get / GET / Get
	if (strcasecmp(method, "get") != 0) {
		// 不是 GET 请求，忽略处理
		printf("用户日觉的不是get请求，忽略...\n");
		return -1;
	}

	//3.判断用户提交的请求是要访问服务器的文件还是目录
	//   /hello/world/
	//   - 第一个 / : 服务器提供的资源根目录，在服务器端可以随意制定
	//   - hello/world/ -> 服务器资源根目录中的两个目录
	//需要在程序中判断得到的文件的属性 - stat()
	//判断path中存储的到底是什么字符串？
	char* file = NULL;		//file中保存的文件路径是相对路径
	if (strcmp(path, "/") == 0) {		//这个 “/”代表的是main中切换的服务器资源根目录
		//访问的是服务器提供的资源根目录			假设：/home/robin/luffy
		// 不是系统根目录，是服务器提供的一个资源目录 == 传递进行的 /home/robin/luffy
		//如何在服务器端将服务器的资源根目录描述出来？
		//在启动服务器程序的时候，先确定资源根目录是哪个目录
		// - 在main函数中将工作目录切换到资源根目录
		file = "./";		//  ./对应的目录就是客户端访问的资源的根目录
	}
	else {
		//假设是这样的：/hello/a.txt
		//  / ==> /home/robin/luffy ==>/home/robin/luffy/hello/a.txt
		//如果不把 / 去掉就相当于要访问系统的根目录
		file = path + 1;	// hello/a.txt == ./hello.a.txt
	}

	//属性判断
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1) {
		//获取文件属性失败--->没有这个文件
		//给客户端发送404
		sendHeadMsg(cfd, 404, "Not Found", getFileType(".jpg"), -1);	// -1:大小不知道，客户端自己计算
		sendFile(cfd, "404.jpg");
	}

	//4.客户端请求的名字是一个目录，遍历目录，发送目录内容给客户端
	if (S_ISDIR(st.st_mode)) {
		//遍历目录，把目录的内容发送给客户端
		sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		sendDir(cfd, file);	//<table></table>
	}

	//5.客户端请求的名字是一个文件，发送文件内容给客户端
	else{
		//目录以外的全为文件，发送文件给客户端
		sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		sendFile(cfd, file);
	}
	return 0;
}

//status：状态码     
//descr：状态描述	
//type：Content-Type的值（要回复的数据的格式）	
//ConTent-Length的值(要回复的数据的长度）
int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int Length)
{
	//状态行 + 消息报头 +空行
	char buf[4096];		//初始化缓冲区
	// http/1.1 200 ok
	sprintf(buf, "http/1.1 %d %s\r\n", status, descr);
	//消息报头 --->2个键值对
	//conten-type：xxx   -----》https://tool.oschina.net/commons
	// mp3--->audio/mp3
	sprintf(buf + strlen(buf), "Content-Type: %s\r\n", type);
	//content-length：111
	//空行
	sprintf(buf + strlen(buf), "Content-Length: %d\r\n\r\n", Length);
	//拼接完成之后，发送
	send(cfd, buf, strlen(buf),0);
	return 0;
}

int sendFile(int cfd, const char* filename)
{
	//在发送内容之前应该有     状态行 + 消息报头 + 空行 + 文件内容
	// 这四部分需要组织好之后再发送吗？
	//  -- 不需要，为什么？   ---》传输层是默认使用tcp
	// 面向连接的流式传输协议  ->只要最后全部发送完就可以
	//读文件内容 ，发送给客户端
	//打开文件
	int fd = open(filename, O_RDONLY);
	//循环读文件
	while (1) {
		char buf[1024] = { 0 };
		int len = read(fd, buf, sizeof(buf));
		if (len > 0) {
			//发送读出的文件内容
			send(cfd, buf, len, 0);
			//发送端发送数据太快会导致接收端的显示有异常
			usleep(50);
		}
		else if (len == 0) {
			//文件读完了
			break;
		}
		else {
			printf("读文件失败...\n");
			return -1;
		}
	}
	return 0;
}

/*
	客户端访问目录，服务器需要遍历当前目录，并且将目录中的所有文件名发送给客户端即可
	- 遍历目录得到的文件名需要放到html表格中
	- 回复的数据是html格式的数据块
	<html>
		<head>
			<title>test</title>
		</head>
		<body>
			<table>
				<tr>
					<td>文件名</td>
					<td>文件大小</td>
				</tr>
			</table>
		</body>
	<html>
*/
// opendir readdir closedir
int sendDir(int cfd, const char* dirName)
{
	char buf[4096];
	struct dirent** namelist;
	sprintf(buf, "<html><head><title>%s</title></head></body><table>", dirName);
	int num = scandir(dirName, &namelist, NULL, alphasort);
	for (int i = 0; i < num; ++i) {
		//取出文件名
		char* name = namelist[i]->d_name;
		//拼接当前文件在资源文件中的相对路径
		char subpath[1024];
		sprintf(subpath, "%s/%s", dirName, name);
		struct stat st;
		//stat函数的第一个参数是文件的路径	
		int ret = stat(subpath, &st);
		if (ret == -1) {
			sendHeadMsg(cfd, 404, "Not Found", getFileType(".jpg"), -1);	// -1:大小不知道，客户端自己计算
			sendFile(cfd, "404.jpg");
		}
		if (S_ISDIR(st.st_mode)) {
			//如果是目录，超链接跳转路径文件后面加/
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
				name, name, (long)st.st_size);
		}
		else {
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
				name, name, (long)st.st_size);
		}
		//发送数据
		send(cfd, buf, strlen(buf), 0);
		//清空数组
		memset(buf, 0, sizeof(buf));
		//释放资源	namelist[i] 这个指针指向一块有效的内存
		free(namelist[i]);
	}
	//补全html剩余的标签
	sprintf(buf, "</table></body></html>");
	send(cfd, buf, strlen(buf), 0);
	//释放namelist
	free(namelist);
	return 0;
}

int disConnect(int cfd, int epfd)
{
	//将cfd从epoll模型上删除
	int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
	if (ret == -1) {
		close(cfd);
		perror("epoll_ctl");
		return -1;
	}
	close(cfd);
	return 0;
}

//通过文件名获取文件的类型
//参数：name-->文件名
//返回值：这个文件对应的Content-Type类型
const char* getFileType(const char* name)
{
	// a.jpg a.mp4 a.thml
	//自右向左查找，'.' 字符，如不存在返回NULL;
	const char* dot = strrchr(name, '.');
	if (dot == NULL)
		return "text/plain; charset=utf-8";		//纯文本
	if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
		return "text/html; charset=utf-8";
	if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
		return "image/jpeg";
	if (strcmp(dot, ".gif") == 0)
		return "image/gif";
	if (strcmp(dot, ".png") == 0)
		return "image/png";
	if (strcmp(dot, ".css") == 0)
		return "text/css";
	if (strcmp(dot, ".au") == 0)
		return "audio/basic";
	if (strcmp(dot, ".wav") == 0)
		return "audio/wav";
	if (strcmp(dot, ".avi") == 0)
		return "video/x-msvideo";
	if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
		return "video/quicktime";
	if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
		return "video/mpeg";
	if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
		return "model/vrml";
	if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
		return "audio/midi";
	if (strcmp(dot, ".mp3") == 0)
		return "audio/mpeg";
	if (strcmp(dot, ".ogg") == 0)
		return "application/ogg";
	if (strcmp(dot, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";

	return "text/plain; charset=utf-8";
}
