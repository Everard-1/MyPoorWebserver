#include "server.h"

//status��״̬��     
//descr��״̬����	
//type��Content-Type��ֵ��Ҫ�ظ������ݵĸ�ʽ��	
//ConTent-Length��ֵ(Ҫ�ظ������ݵĳ��ȣ�
int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int Length)
{
	//״̬�� + ��Ϣ��ͷ +����
	char buf[4096];		//��ʼ��������
	// http/1.1 200 ok
	sprintf(buf, "http/1.1 %d %s\r\n", status, descr);
	//��Ϣ��ͷ --->2����ֵ��
	//conten-type��xxx   -----��https://tool.oschina.net/commons
	// mp3--->audio/mp3
	sprintf(buf + strlen(buf), "Content-Type: %s\r\n", type);
	//content-length��111
	//����
	sprintf(buf + strlen(buf), "Content-Length: %d\r\n\r\n", Length);
	//ƴ�����֮�󣬷���
	send(cfd, buf, strlen(buf), 0);
	return 0;
}