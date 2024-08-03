#include "server.h"
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>

/*
	�ͻ��˷���Ŀ¼����������Ҫ������ǰĿ¼�����ҽ�Ŀ¼�е������ļ������͸��ͻ��˼���
	- ����Ŀ¼�õ����ļ�����Ҫ�ŵ�html�����
	- �ظ���������html��ʽ�����ݿ�
	<html>
		<head>
			<title>test</title>
		</head>
		<body>
			<table>
				<tr>
					<td>�ļ���</td>
					<td>�ļ���С</td>
				</tr>
			</table>
		</body>
	<html>
*/
// opendir readdir closedir
int sendDir(int cfd, const char* dirName)
{
	char buf[4096];
	struct dirent** namelist;
	sprintf(buf, "<html><head><title>%s</title></head></body><table>", dirName);
	int num = scandir(dirName, &namelist, NULL, alphasort);
	for (int i = 0; i < num; ++i) {
		//ȡ���ļ���
		char* name = namelist[i]->d_name;
		//ƴ�ӵ�ǰ�ļ�����Դ�ļ��е����·��
		char subpath[1024];
		sprintf(subpath, "%s/%s", dirName, name);
		struct stat st;
		//stat�����ĵ�һ���������ļ���·��	
		int ret = stat(subpath, &st);
		if (ret == -1) {
			sendHeadMsg(cfd, 404, "Not Found", getFileType(".jpg"), -1);	// -1:��С��֪�����ͻ����Լ�����
			sendFile(cfd, "404.jpg");
		}
		if (S_ISDIR(st.st_mode)) {
			//�����Ŀ¼����������ת·���ļ������/
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
				name, name, (long)st.st_size);
		}
		else {
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
				name, name, (long)st.st_size);
		}
		//��������
		send(cfd, buf, strlen(buf), 0);
		//�������
		memset(buf, 0, sizeof(buf));
		//�ͷ���Դ	namelist[i] ���ָ��ָ��һ����Ч���ڴ�
		free(namelist[i]);
	}
	//��ȫhtmlʣ��ı�ǩ
	sprintf(buf, "</table></body></html>");
	send(cfd, buf, strlen(buf), 0);
	//�ͷ�namelist
	free(namelist);
	return 0;
}