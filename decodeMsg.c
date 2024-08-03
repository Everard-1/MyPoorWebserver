#include "server.h"

//解码
//from：要被转换的字符	----->传入参数
//to：转换之后得到的字符 --->传出参数
void decodeMsg(char* to, char* from)
{
	for (; *from != '\0'; ++to, ++from) {
		//isxdigit--->判断字符是不是16进制格式
		//Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {
			//将16进制的数-->十进制 将这个数值赋给了字符 int->char
			// A1 == 161
			*to = hexit(from[1]) * 16 + hexit(from[2]);
			from += 2;
		}
		else
			//不是特殊字符字节赋值
			*to = *from;
	}
	*to = '\0';
}