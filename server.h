#pragma once
//������Ҫ�����ҵ���߼�

// ��ʼ���������ļ�������
int initListenFd(unsigned short port);
//����epollģ��
int epollRun(unsigned short port);
//�Ϳͻ��˽�������
int acceptConn(int lfd, int epfd);
//���տͻ��˵�http������Ϣ
int recvHttpRequest(int cfd);
//����������
int parseRequestLine(const char* reqLine);
