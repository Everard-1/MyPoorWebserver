#include <stdio.h>
#include "server.h"

//原则：main函数只是逻辑调用，具体函数不会太复杂
//功能函数：功能尽可能要单一，一个函数的代码一般最多也就写几十行
int main(int argc, char* argv[])
{
	//命令行传参形式：a.out port
	if (argc < 2) {
		printf("./a.out port\n");
		exit(0);
	}
	//启动服务器->基于epoll
	unsigned short port = aoti(argv[1]);	//获取端口
	epollRun(port);
	return 0;
}