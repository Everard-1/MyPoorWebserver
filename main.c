#include <stdio.h>
#include "server.h"

//ԭ��main����ֻ���߼����ã����庯������̫����
//���ܺ��������ܾ�����Ҫ��һ��һ�������Ĵ���һ�����Ҳ��д��ʮ��
int main(int argc, char* argv[])
{
	//�����д�����ʽ��a.out port
	if (argc < 2) {
		printf("./a.out port\n");
		exit(0);
	}
	//����������->����epoll
	unsigned short port = aoti(argv[1]);	//��ȡ�˿�
	epollRun(port);
	return 0;
}