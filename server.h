#pragma once
//服务器要处理的业务逻辑

// 初始化监听的文件描述符
int initListenFd(unsigned short port);
//启动epoll模型
int epollRun(unsigned short port);
//和客户端建立连接
int acceptConn(int lfd, int epfd);
//接收客户端的http请求消息
int recvHttpRequest(int cfd,int epfd);
//解析请求行
int parseRequestLine(int cfd, const char* reqLine);
//发送头信息  （状态行 + 消息报头 + 空行）
int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int Length);
//读文件内容,并发送
int sendFile(int cfd, const char* filename);
//发送目录给客户端
int sendDir(int cfd, const char* dirName);
//和客户端断开连接
int disConnect(int cfd, int epfd);
//通过文件名获取到文件的类型
const char* getFileType(const char* name);
//发送目录给客户端
int sendDir(int cfd, const char* dirName);
//中文解码
int hexit(char c);
void decodeMsg(char* to, char* from);

