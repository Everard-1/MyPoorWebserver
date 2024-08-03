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
				recvHttpRequest(curfd, epfd);
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
	}
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
	else{
		//Ŀ¼�����ȫΪ�ļ��������ļ����ͻ���
		sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		sendFile(cfd, file);
	}
	return 0;
}

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
	send(cfd, buf, strlen(buf),0);
	return 0;
}

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

int disConnect(int cfd, int epfd)
{
	//��cfd��epollģ����ɾ��
	int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
	if (ret == -1) {
		close(cfd);
		perror("epoll_ctl");
		return -1;
	}
	close(cfd);
	return 0;
}

//ͨ���ļ�����ȡ�ļ�������
//������name-->�ļ���
//����ֵ������ļ���Ӧ��Content-Type����
const char* getFileType(const char* name)
{
	// a.jpg a.mp4 a.thml
	//����������ң�'.' �ַ����粻���ڷ���NULL;
	const char* dot = strrchr(name, '.');
	if (dot == NULL)
		return "text/plain; charset=utf-8";		//���ı�
	if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
		return "text/html; charset=utf-8";
	if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
		return "image/jpeg";
	if (strcmp(dot, ".gif") == 0)
		return "image/gif";
	if (strcmp(dot, ".png") == 0)
		return "image/png";
	if (strcmp(dot, ".css") == 0)
		return "text/css";
	if (strcmp(dot, ".au") == 0)
		return "audio/basic";
	if (strcmp(dot, ".wav") == 0)
		return "audio/wav";
	if (strcmp(dot, ".avi") == 0)
		return "video/x-msvideo";
	if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
		return "video/quicktime";
	if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
		return "video/mpeg";
	if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
		return "model/vrml";
	if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
		return "audio/midi";
	if (strcmp(dot, ".mp3") == 0)
		return "audio/mpeg";
	if (strcmp(dot, ".ogg") == 0)
		return "application/ogg";
	if (strcmp(dot, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";

	return "text/plain; charset=utf-8";
}
