#include "server.h"
#include <fcntl.h>


int sendFile(int cfd, const char* filename)
{
	//�ڷ�������֮ǰӦ����     ״̬�� + ��Ϣ��ͷ + ���� + �ļ�����
	// ���Ĳ�����Ҫ��֯��֮���ٷ�����
	//  -- ����Ҫ��Ϊʲô��   ---���������Ĭ��ʹ��tcp
	// �������ӵ���ʽ����Э��  ->ֻҪ���ȫ��������Ϳ���
	//���ļ����� �����͸��ͻ���
	//���ļ�
	int fd = open(filename, O_RDONLY);
	//ѭ�����ļ�
	while (1) {
		char buf[1024] = { 0 };
		int len = read(fd, buf, sizeof(buf));
		if (len > 0) {
			//���Ͷ������ļ�����
			send(cfd, buf, len, 0);
			//���Ͷ˷�������̫��ᵼ�½��ն˵���ʾ���쳣
			usleep(50);
		}
		else if (len == 0) {
			//�ļ�������
			break;
		}
		else {
			printf("���ļ�ʧ��...\n");
			return -1;
		}
	}
	return 0;
}
