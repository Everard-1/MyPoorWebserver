#include "server.h"

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
	send(cfd, buf, strlen(buf), 0);
	return 0;
}