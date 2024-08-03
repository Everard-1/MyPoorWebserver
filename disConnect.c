#include "server.h"
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>


int disConnect(int cfd, int epfd)
{
	//��cfd��epollģ����ɾ��
	int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
	if (ret == -1) {
		close(cfd);
		perror("epoll_ctl");
		return -1;
	}
	close(cfd);
	return 0;
}
