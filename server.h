#pragma once
//������Ҫ�����ҵ���߼�

// ��ʼ���������ļ�������
int initListenFd(unsigned short port);
//����epollģ��
int epollRun(unsigned short port);
//�Ϳͻ��˽�������
int acceptConn(int lfd, int epfd);
