#include "server.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>



int parseRequestLine(int cfd, const char* reqLine)
{
	//请求行分为三部分
	//GET /hello/world/ http/1.1
	//1.将请求行的三部分依次拆分，有用的前两部分
	// - 提交数据的方式
	// - 客户端向服务器请求的文件名
	char method[6];
	char path[1024];
	sscanf(reqLine, "%[^ ] %[^ ]", method, path);

	//2.判断请求方式是不是get，不是get方式直接忽略
	//http中不区分大小写 get / GET / Get
	if (strcasecmp(method, "get") != 0) {
		// 不是 GET 请求，忽略处理
		printf("用户日觉的不是get请求，忽略...\n");
		return -1;
	}

	//3.判断用户提交的请求是要访问服务器的文件还是目录
	//   /hello/world/
	//   - 第一个 / : 服务器提供的资源根目录，在服务器端可以随意制定
	//   - hello/world/ -> 服务器资源根目录中的两个目录
	//需要在程序中判断得到的文件的属性 - stat()
	//判断path中存储的到底是什么字符串？
	char* file = NULL;		//file中保存的文件路径是相对路径
	//如果文件名有中文，需要还原
	decodeMsg(path, path);
	if (strcmp(path, "/") == 0) {		//这个 “/”代表的是main中切换的服务器资源根目录
		//访问的是服务器提供的资源根目录			假设：/home/robin/luffy
		// 不是系统根目录，是服务器提供的一个资源目录 == 传递进行的 /home/robin/luffy
		//如何在服务器端将服务器的资源根目录描述出来？
		//在启动服务器程序的时候，先确定资源根目录是哪个目录
		// - 在main函数中将工作目录切换到资源根目录
		file = "./";		//  ./对应的目录就是客户端访问的资源的根目录
	}
	else {
		//假设是这样的：/hello/a.txt
		//  / ==> /home/robin/luffy ==>/home/robin/luffy/hello/a.txt
		//如果不把 / 去掉就相当于要访问系统的根目录
		file = path + 1;	// hello/a.txt == ./hello.a.txt
	}

	printf("客户端请求的文件名: %s\n", file);

	//属性判断
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1) {
		//获取文件属性失败--->没有这个文件
		//给客户端发送404
		sendHeadMsg(cfd, 404, "Not Found", getFileType(".jpg"), -1);	// -1:大小不知道，客户端自己计算
		sendFile(cfd, "404.jpg");
	}

	//4.客户端请求的名字是一个目录，遍历目录，发送目录内容给客户端
	if (S_ISDIR(st.st_mode)) {
		//遍历目录，把目录的内容发送给客户端
		sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		sendDir(cfd, file);	//<table></table>
	}

	//5.客户端请求的名字是一个文件，发送文件内容给客户端
	else {
		//目录以外的全为文件，发送文件给客户端
		sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		sendFile(cfd, file);
	}
	return 0;
}
