#include "server.h"
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/stat.h>
#include <strings.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>

int recvHttpRequest(int cfd,int epfd)
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
		// �����ַ�������������һ��\r\n��ʱ����ζ���������õ���
		char* pt = strstr(buf, "\r\n");
		//���������г���
		int reqlen = pt - buf;
		//���������оͿ���
		buf[reqlen] = '\0';		//�ַ����ض�
		//����������
		parseRequestLine(cfd,buf);
	}
	else if (len == 0) {
		printf("�ͻ��˶Ͽ�������...\n");
		//�������Ϳͻ��˶Ͽ����ӣ��ļ���������epollģ����ɾ��
		disConnect(cfd, epfd);
	}
	else {
		perror("recv");
		return -1;
	}
	return 0;
}
