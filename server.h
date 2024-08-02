#pragma once
//服务器要处理的业务逻辑

// 初始化监听的文件描述符
int initListenFd(unsigned short port);
//启动epoll模型
int epollRun();
