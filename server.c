#include "server.h"
#include <arpa/inet.h>
#include <sys/epoll.h>

int initListenFd(unsigned short port)
{
	//1.�����������׽���
	int lfd = socket(AF_INET, SOCK_STREAM, 0);	//ʹ�� IPv4 (AF_INET)���������ӵ� TCP Э�� (SOCK_STREAM)
	if (lfd == -1) {
		perror("socket");
		return -1;
	}
	//2.���ö˿ڸ���
	int opt = 1;	//��ʾ����
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));		//SOL_SOCKET ��ʾͨ���׽���ѡ�SO_REUSEADDR ����˿ڸ���
	if (ret == -1) {
		perror("setsockopt");
		return -1;
	}
	//3.��
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);	//���ö˿ںţ�ʹ�� htons �������ֽ���ת��Ϊ�����ֽ���
	addr.sin_addr.s_addr = INADDR_ANY;
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));		//����ַ�Ͷ˿ڰ󶨵��׽���
	if (ret == -1) {
		perror("bind");
		return -1;
	}
	//4.���ü���
	ret = listen(lfd, 128);		//���׽�������Ϊ��������ģʽ��׼���������ӡ��ڶ������� 128 ָ�������δ�����Ӷ��еĳ���
	if (ret == -1) {
		perror("listen");
		return -1;
	}
	//5.���ش������󶨺õļ����׽����ļ�������
	return lfd;
}

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
	while (1) {
		int num = epoll_wait(epfd, evs, size, -1);		//�ȴ� epoll �¼���-1 ��ʾ���޵ȴ�
		for (int i = 0; i < num; i++) {
			int curfd = evs[i].data.fd;		//��ȡ��ǰ�¼����ļ�������
			if (curfd == lfd) {
				//����������
			}
			else {
				//ͨ��->�Ƚ������ݣ�Ȼ���ٻظ�����
			}
		}
	}
	return 0;
}
