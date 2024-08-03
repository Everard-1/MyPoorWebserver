#include "server.h"
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>



int acceptConn(int lfd, int epfd)
{
	//1.����������
	int cfd = accept(lfd, NULL, NULL);	//����һ���µĿͻ������ӣ��������µĿͻ����׽����ļ������� cfd
	if (cfd == -1) {
		perror("accept");
		return -1;
	}
	//2.����ͨ��������Ϊ������
	int flag = fcntl(cfd, F_GETFL);		//��ȡ��ǰ�ļ������� cfd �ı�־��
	flag |= O_NONBLOCK;		//���ļ�����������Ϊ������ģʽ
	fcntl(cfd, F_SETFL, flag);		//���޸ĺ�ı�־���û��ļ������� cfd
	//3.ͨ�ŵ��ļ���������ӵ�epollģ����
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;		//��ʾ�����ݿɶ��Ҳ��ñ��ش���ģʽ
	ev.data.fd = cfd;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);		//���µĿͻ����ļ���������ӵ� epoll ʵ�� epfd �н����¼����
	if (ret == -1) {
		perror("epoll_ctl");
		return -1;
	}
	return 0;
}
