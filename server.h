#pragma once
//������Ҫ�����ҵ���߼�

// ��ʼ���������ļ�������
int initListenFd(unsigned short port);
//����epollģ��
int epollRun(unsigned short port);
//�Ϳͻ��˽�������
int acceptConn(int lfd, int epfd);
//���տͻ��˵�http������Ϣ
int recvHttpRequest(int cfd,int epfd);
//����������
int parseRequestLine(int cfd, const char* reqLine);
//����ͷ��Ϣ  ��״̬�� + ��Ϣ��ͷ + ���У�
int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int Length);
//���ļ�����,������
int sendFile(int cfd, const char* filename);
//����Ŀ¼���ͻ���
int sendDir(int cfd, const char* dirName);
//�Ϳͻ��˶Ͽ�����
int disConnect(int cfd, int epfd);
//ͨ���ļ�����ȡ���ļ�������
const char* getFileType(const char* name);
//����Ŀ¼���ͻ���
int sendDir(int cfd, const char* dirName);
//���Ľ���
int hexit(char c);
void decodeMsg(char* to, char* from);

