#include "server.h"

//����
//from��Ҫ��ת�����ַ�	----->�������
//to��ת��֮��õ����ַ� --->��������
void decodeMsg(char* to, char* from)
{
	for (; *from != '\0'; ++to, ++from) {
		//isxdigit--->�ж��ַ��ǲ���16���Ƹ�ʽ
		//Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {
			//��16���Ƶ���-->ʮ���� �������ֵ�������ַ� int->char
			// A1 == 161
			*to = hexit(from[1]) * 16 + hexit(from[2]);
			from += 2;
		}
		else
			//���������ַ��ֽڸ�ֵ
			*to = *from;
	}
	*to = '\0';
}