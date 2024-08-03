#include "server.h"
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>

int epollRun(unsigned short port)
{
	//1.����epollģ��
	int epfd = epoll_create(10);	//���� 10 ����ʾ�ں���Ҫ���ٸ��ļ�����������ɲ�����ʵ�������Ѿ��������ˣ�ֻ��Ҫ����0����
	if (epfd == -1) {
		perror("epoll_create");
		return -1;
	}
	//2.��ʼ��epollģ��
	int lfd = initListenFd(port);	//��ʼ��һ�������׽��֣��󶨵�ָ���˿ڲ���ʼ��������
	struct epoll_event ev;		//����һ�� epoll �¼��ṹ��
	ev.events = EPOLLIN;	//�����¼�����Ϊ EPOLLIN����ʾ�����ݿɶ�
	ev.data.fd = lfd;		//�����¼����ļ�������Ϊ�����׽��ֵ��ļ������� lfd
	//���lfd �����ģ����
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);		//�������׽��� lfd ��ӵ� epoll ʵ�� epfd �У�����ע�� EPOLLIN �¼���
	if (epfd == -1) {
		perror("epoll_ctl");
		return -1;
	}
	//��� - ѭ�����
	struct epoll_event evs[1024];		//����һ������ evs�����ڴ洢 epoll ���ص��¼�
	int size = sizeof(evs) / sizeof(evs[0]);
	int flag = 0;
	while (1) {
		if (flag) {
			break;
		}
		//���̲߳�ͣ�ĵ���epoll_wait
		int num = epoll_wait(epfd, evs, size, -1);		//�ȴ� epoll �¼���-1 ��ʾ���޵ȴ�
		for (int i = 0; i < num; i++) {
			int curfd = evs[i].data.fd;		//��ȡ��ǰ�¼����ļ�������
			if (curfd == lfd) {
				//����������
				//�������߳�,�����߳��н����µ�����
				//acceptConn�����̵߳Ļص�
				int ret = acceptConn(lfd, epfd);
				if (ret == -1) {
					//�涨����������ʧ�ܣ�ֱ����ֹ����
					flag = 1;
					break;
				}
			}
			else {
				//ͨ��->�Ƚ������ݣ�Ȼ���ٻظ�����
				//�����̣߳������߳���ͨ�ţ�recvHttpRequestҲ�����̵߳Ļص�
				recvHttpRequest(curfd, epfd);
			}
		}
	}
	return 0;
}
