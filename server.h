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
int parseRequestLine(const char* reqLine);
//����ͷ��Ϣ  ��״̬�� + ��Ϣ��ͷ + ���У�
int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int Length);
//���ļ�����,������
int sendFile(const char* filename);
//�Ϳͻ��˶Ͽ�����
int disConnect(int cfd, int epfd);
