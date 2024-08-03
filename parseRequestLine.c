#include "server.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>



int parseRequestLine(int cfd, const char* reqLine)
{
	//�����з�Ϊ������
	//GET /hello/world/ http/1.1
	//1.�������е����������β�֣����õ�ǰ������
	// - �ύ���ݵķ�ʽ
	// - �ͻ����������������ļ���
	char method[6];
	char path[1024];
	sscanf(reqLine, "%[^ ] %[^ ]", method, path);

	//2.�ж�����ʽ�ǲ���get������get��ʽֱ�Ӻ���
	//http�в����ִ�Сд get / GET / Get
	if (strcasecmp(method, "get") != 0) {
		// ���� GET ���󣬺��Դ���
		printf("�û��վ��Ĳ���get���󣬺���...\n");
		return -1;
	}

	//3.�ж��û��ύ��������Ҫ���ʷ��������ļ�����Ŀ¼
	//   /hello/world/
	//   - ��һ�� / : �������ṩ����Դ��Ŀ¼���ڷ������˿��������ƶ�
	//   - hello/world/ -> ��������Դ��Ŀ¼�е�����Ŀ¼
	//��Ҫ�ڳ������жϵõ����ļ������� - stat()
	//�ж�path�д洢�ĵ�����ʲô�ַ�����
	char* file = NULL;		//file�б�����ļ�·�������·��
	//����ļ��������ģ���Ҫ��ԭ
	decodeMsg(path, path);
	if (strcmp(path, "/") == 0) {		//��� ��/���������main���л��ķ�������Դ��Ŀ¼
		//���ʵ��Ƿ������ṩ����Դ��Ŀ¼			���裺/home/robin/luffy
		// ����ϵͳ��Ŀ¼���Ƿ������ṩ��һ����ԴĿ¼ == ���ݽ��е� /home/robin/luffy
		//����ڷ������˽�����������Դ��Ŀ¼����������
		//�����������������ʱ����ȷ����Դ��Ŀ¼���ĸ�Ŀ¼
		// - ��main�����н�����Ŀ¼�л�����Դ��Ŀ¼
		file = "./";		//  ./��Ӧ��Ŀ¼���ǿͻ��˷��ʵ���Դ�ĸ�Ŀ¼
	}
	else {
		//�����������ģ�/hello/a.txt
		//  / ==> /home/robin/luffy ==>/home/robin/luffy/hello/a.txt
		//������� / ȥ�����൱��Ҫ����ϵͳ�ĸ�Ŀ¼
		file = path + 1;	// hello/a.txt == ./hello.a.txt
	}

	printf("�ͻ���������ļ���: %s\n", file);

	//�����ж�
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1) {
		//��ȡ�ļ�����ʧ��--->û������ļ�
		//���ͻ��˷���404
		sendHeadMsg(cfd, 404, "Not Found", getFileType(".jpg"), -1);	// -1:��С��֪�����ͻ����Լ�����
		sendFile(cfd, "404.jpg");
	}

	//4.�ͻ��������������һ��Ŀ¼������Ŀ¼������Ŀ¼���ݸ��ͻ���
	if (S_ISDIR(st.st_mode)) {
		//����Ŀ¼����Ŀ¼�����ݷ��͸��ͻ���
		sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		sendDir(cfd, file);	//<table></table>
	}

	//5.�ͻ��������������һ���ļ��������ļ����ݸ��ͻ���
	else {
		//Ŀ¼�����ȫΪ�ļ��������ļ����ͻ���
		sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		sendFile(cfd, file);
	}
	return 0;
}
