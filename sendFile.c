#include "server.h"
#include <fcntl.h>


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
