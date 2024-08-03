#include "server.h"
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>

/*
	客户端访问目录，服务器需要遍历当前目录，并且将目录中的所有文件名发送给客户端即可
	- 遍历目录得到的文件名需要放到html表格中
	- 回复的数据是html格式的数据块
	<html>
		<head>
			<title>test</title>
		</head>
		<body>
			<table>
				<tr>
					<td>文件名</td>
					<td>文件大小</td>
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
		//取出文件名
		char* name = namelist[i]->d_name;
		//拼接当前文件在资源文件中的相对路径
		char subpath[1024];
		sprintf(subpath, "%s/%s", dirName, name);
		struct stat st;
		//stat函数的第一个参数是文件的路径	
		int ret = stat(subpath, &st);
		if (ret == -1) {
			sendHeadMsg(cfd, 404, "Not Found", getFileType(".jpg"), -1);	// -1:大小不知道，客户端自己计算
			sendFile(cfd, "404.jpg");
		}
		if (S_ISDIR(st.st_mode)) {
			//如果是目录，超链接跳转路径文件后面加/
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
				name, name, (long)st.st_size);
		}
		else {
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
				name, name, (long)st.st_size);
		}
		//发送数据
		send(cfd, buf, strlen(buf), 0);
		//清空数组
		memset(buf, 0, sizeof(buf));
		//释放资源	namelist[i] 这个指针指向一块有效的内存
		free(namelist[i]);
	}
	//补全html剩余的标签
	sprintf(buf, "</table></body></html>");
	send(cfd, buf, strlen(buf), 0);
	//释放namelist
	free(namelist);
	return 0;
}