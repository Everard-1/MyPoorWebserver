#include "server.h"
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>

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
	int flag = 0;
	while (1) {
		if (flag) {
			break;
		}
		int num = epoll_wait(epfd, evs, size, -1);		//�ȴ� epoll �¼���-1 ��ʾ���޵ȴ�
		for (int i = 0; i < num; i++) {
			int curfd = evs[i].data.fd;		//��ȡ��ǰ�¼����ļ�������
			if (curfd == lfd) {
				//����������
				int ret = acceptConn(lfd, epfd);
				if (ret == -1) {
					//�涨����������ʧ�ܣ�ֱ����ֹ����
					flag = 1;
					break;
				}
			}
			else {
				//ͨ��->�Ƚ������ݣ�Ȼ���ٻظ�����
			}
		}
	}
	return 0;
}

int acceptConn(int lfd, int epfd)
{
	//1.����������
	int cfd = accept(lfd, NULL, NULL);	//����һ���µĿͻ������ӣ��������µĿͻ����׽����ļ������� cfd
	if (cfd == -1) {
		perror("accept");
		return -1;
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
}

int recvHttpRequest(int cfd)
{
	//��ʱ���������ڴ洢���׽��ֶ�ȡ������
	char tmp[1024];		//ÿ�ν���1k����
	char buf[4096];		//��ÿ�ζ��������ݴ洢�����buf��

	//ѭ��������
	int len, total = 0;	//total����ǰbuf���Ѿ��洢�˶�������

	//û�б�Ҫ�����е�http����ȫ����������
	//��Ϊ��Ҫ�����ݶ�����������
	//-�ͻ��������������Ķ��Ǿ�̬��Դ���������Դ�����������еĵڶ�����
	//-ֻ��Ҫ�����������ı��������Ϳ��ԣ������к������ͷ�Ϳ���
	//-����Ҫ��������ͷ�е����ݣ���˽��յ� ֮�󲻴洢Ҳ��û�����
	while ((len = recv(cfd, tmp, sizeof(tmp), 0)) > 0){			//ʹ�� recv ������ȡ���ݵ� tmp ������
		if (total + len < sizeof(buf)) {
			//�пռ������
			memcpy(buf + total, tmp, len);
		}
		total += len;	//����total
	}

	//ѭ������--->������
	//�������Ƿ������ģ���ǰ������û������ֵ����-1��errno==EAGAIN
	if (len == -1 && errno == EAGAIN) {
		//�������дӽ��ܵ��������ó���
		//��http�л���ʹ�õ���\r\n
		// �����ַ����������һ��\r\n��ʱ����ζ���������õ���
		char* pt = strstr(buf, "\r\n");
		//���������г���
		int reqlen = pt - buf;
		//���������оͿ���
		buf[reqlen] = '\0';		//�ַ����ض�
		//����������

	}
	else if (len == 0) {
		printf("�ͻ��˶Ͽ�������...\n");
		//�������Ϳͻ��˶Ͽ����ӣ��ļ���������epollģ����ɾ��
	}
	else {
		perror("recv");
		return -1;
	}
	return 0;
}

int parseRequestLine(const char* reqLine)
{
	//�����з�Ϊ������
	//GET /hello/world/ http/1.1
	//1.�������е����������β�֣����õ�ǰ������
	// - �ύ���ݵķ�ʽ
	// - �ͻ����������������ļ���

	//2.�ж�����ʽ�ǲ���get������get��ʽֱ�Ӻ���

	//3.�ж��û��ύ��������Ҫ���ʷ��������ļ�����Ŀ¼
	//   /hello/world/
	//   - ��һ�� / : �������ṩ����Դ��Ŀ¼���ڷ������˿��������ƶ�
	//   - hello/world/ -> ��������Դ��Ŀ¼�е�����Ŀ¼
	//��Ҫ�ڳ������жϵõ����ļ������� - stat()

	//4.�ͻ��������������һ���ļ��������ļ����ݸ��ͻ���
	
	//5.�ͻ��������������һ��Ŀ¼������Ŀ¼������Ŀ¼���ݸ��ͻ���
	return 0;
}
