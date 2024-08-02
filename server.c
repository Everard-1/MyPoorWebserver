#include "server.h"
#include <arpa/inet.h>

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

int epollRun()
{
	//1.����epollģ��
	epoll_create();
	epoll_ctl();
	//���

	return 0;
}
