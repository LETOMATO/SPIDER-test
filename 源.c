#include<stdio.h>
#include<string.h>
#include<WinSock2.h>
#include<rpcssl.h>
#pragma comment(lib,"ws2_32.lib")//Windows Sockets应用程序接口， 用于支持Internet和网络应用程序
#pragma comment(lib,"Urlmon.lib")
/*
网络部分：http url
url分为三个部分
http://www.netbian.com/desk/7535.htm
1.协议 http超文本传输协议(规则)
2.主机名 www.netbian.com需要的IP地址 183.61.190.207
3.资源名 /desk/7535.htm
*/
void parseUrl(const char* url, char* host, char* resPath);
void getImagUrl(char* imgUrl);
typedef struct Spider
{
	char host[128];
	char resPath[128];
	SOCKET fd;
}Spider;
//获取资源
void Spider_init(Spider* spider, const char* url)
{
	memset(spider->host, 0, sizeof(spider->host));
	memset(spider->resPath, 0, sizeof(spider->resPath));
	parseUrl(url, spider->host, spider->resPath);
}
//连接到服务器
void Spider_connect(Spider* spider)
{
	//网络编程: 打开socket 2.2 初始化 Winsock. 创建名为 wsaData 的 WSADATA 对象
	WSADATA wsadata;//传出参数
	int err=WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* Winsock DLL.                                  */
		printf("WSAStartup failed with error: %d\n", err);
		return 1;
	}
	//创建socket
	spider->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//af:地址协议的族 此处用IPv4
	if (spider->fd == SOCKET_ERROR)
	{
		printf("creat socket failed %d\n", WSAGetLastError());
		return;
	}
	//通过域名获取IP地址
	HOSTENT* hent = gethostbyname(spider->host);//从主机数据库中检索与主机名对应的主机信息
	if (!hent) {
		printf("get host ip failed %d\n", WSAGetLastError());
		return;
	}
	//连接服务器
	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;//传输地址的地址系列。 此成员应始终设置为AF_INET。
	addr.sin_port = htons(80); //端口号 http:80 端序 本地字节序转化为网络字节序
	memcpy(&addr.sin_addr, hent->h_addr, sizeof(IN_ADDR));//包含 IPv4 传输地址的 IN_ADDR 结构(表示 IPv4 Internet 地址。)。
	if (SOCKET_ERROR == connect(spider->fd, &addr, sizeof(addr)))
	{
		printf("connect failed %d\n", WSAGetLastError());
		return;
	}
}
//解析url,获取主机名和资源路径
void parseUrl(const char* url, char* host, char* resPath)
{
	if (!url) {
		return;
	}
	//先获取host
	const char* ph = strstr(url, "//");//查找子串
	ph = ph ? ph + 2 : url;//if (!ph) {ph = url;}else {ph += 2;}
	puts(ph);
	const char* pp = strstr(ph, "/");
	if (!pp) {
		strcpy_s(host, sizeof(host), ph);
		strcpy_s(resPath, sizeof(resPath), "/");//index.html
	}
	else {
		strncpy_s(host, 128, ph, pp - ph);
		strcpy_s(resPath, 128, pp);
	}
}
//获取网页
void getHtml(Spider* spider)
{
	//连接到服务器
	Spider_connect(spider);
	//给服务器发送请求
	char header[400] = { 0 };
	char get[128] = { 0 }; strcpy_s(get, _countof(get), "GET %s HTTP/1.1\r\n");
	char host[128] = { 0 }; strcpy_s(host, _countof(host), "Host:%s\r\n");
	char con[128] = { 0 }; strcpy_s(con, _countof(con), "Connection:Close\r\nContent-Type: text/html\r\n");//Connection:Close\r\nContent-Type: image/jpeg\r\n
	char user[200] = { 0 }; strcpy_s(user, _countof(user), "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"
		"(KHTML, like Gecko) Chrome/112.0.0.0 Safari/537.36 Edg/112.0.1722.39\r\n");
	char rn[128] = { 0 }; strcpy_s(rn, _countof(rn), "\r\n");
	size_t h_len = strlen(get) + strlen(spider->resPath) + 1;
	sprintf_s(header, h_len, "GET %s HTTP/1.1\r\n", spider->resPath);
	h_len += strlen(host) + strlen(spider->host);
	sprintf_s(header + strlen(header), h_len, "Host:%s\r\n", spider->host);
	h_len += strlen(con);
	strcat_s(header, h_len, con);
	h_len += strlen(user);
	strcat_s(header, h_len, user);
	h_len += strlen(rn);
	strcat_s(header, h_len, rn);

	if (SOCKET_ERROR == send(spider->fd, header, strlen(header), 0))
	{
		printf("send falied %d\n", WSAGetLastError());
		return;
	}
	//获取网页数据
	char html[1024] = { 0 };
	FILE* fp;
	errno_t err = fopen_s(&fp, "maye.html", "w");
	if (!fp)
		return;
	int total = 0,n=0;
	n = recv(spider->fd, html, 1023, 0);
	html[n] = "\0";
	char* end = strstr(html, "\r\n\r\n");
	if (end != NULL) {
		end += strlen("\r\n\r\n");
		fwrite(end, sizeof(char), n - (end - html), fp);
		//strcpy_s(html, _countof(html), end);
		printf("%s", html);
		total += n;
	}
	while ((n=recv(spider->fd, html, 1023, 0))>0)
	{
		html[n] = '\0';
		printf("%s", html);
		fwrite(html, sizeof(char), strlen(html), fp);
		total += n;
	}
	if (n < 0) {
		perror("recv error");
		exit(EXIT_FAILURE);
	}
	printf("\nTotal received: %d bytes\n", total);
	fclose(fp);
	char imgUrl[128] = {0};
	getImagUrl(imgUrl);
	Spider sp;
	Spider_init(&sp, imgUrl);
	//连接服务器
	Spider_connect(&sp);
	//发送请求
	memset(con, 0, 128);
	strcpy_s(con, _countof(con), "Connection:Close\r\nContent-Type: image/jpeg\r\n");
	h_len = strlen(get) + strlen(sp.resPath) + 1;
	sprintf_s(header, h_len, "GET %s HTTP/1.1\r\n", sp.resPath);
	h_len += strlen(host) + strlen(sp.host);
	sprintf_s(header + strlen(header), h_len, "Host:%s\r\n", sp.host);
	h_len += strlen(con);
	strcat_s(header, h_len, con);
	h_len += strlen(user);
	strcat_s(header, h_len, user);
	h_len += strlen(rn);
	strcat_s(header, h_len, rn);

	if (SOCKET_ERROR == send(sp.fd, header, strlen(header), 0))
	{
		printf("send falied %d\n", WSAGetLastError());
		return;
	}
	//获取图片
	char jpg[1024] = { 0 };
	int i = 0;
	int chs = 0;
	err = fopen_s(&fp, "imgUrl.png", "wb");
	if (!fp)
		return;
	while ((chs=recv(sp.fd, jpg,1023, 0))>0)
	{
		jpg[chs] = "\0";
		char* end = strstr(jpg, "\r\n\r\n");
		if (end != NULL) {
			end += strlen("\r\n\r\n");
			//strcpy_s(jpg, _countof(jpg), end);
			fwrite(end, sizeof(char), chs - (end - jpg), fp);
			continue;
		}
		//printf("%s", jpg);
		fwrite(jpg, 1, chs, fp);		
	}
	if (chs == SOCKET_ERROR)
	{
		printf("recv failed %d\n", WSAGetLastError());
	}
	fclose(fp);
}
//获取网页里面的图片连接
void getImagUrl(char* imgUrl)
{
	FILE* pfile;
	char* data;
	char* starts=0;
	int length=0;
	errno_t err=fopen_s(&pfile,"maye.html","r");
	if (pfile == NULL)
	{
		return NULL;
	}
	fseek(pfile, 0, SEEK_END);
	length = ftell(pfile);
	data = (char*)malloc((length) * sizeof(char));
	rewind(pfile);
	if (data != 0)
	{
		length = fread(data, 1, length - 1, pfile);
		data[length] = '\0';
		fclose(pfile);
		starts = strstr(data, "<img src=\"");//<img src=\"
	}
	if (!starts) {
		return;
	}
	else {
		printf("\n\n\n\n\n");
		starts += 10;
	}
	//结尾双引号
	char* end = strstr(starts, "\"");
	if (!end) {
		printf("网页错误\n");
	}
	else {
		strncpy_s(imgUrl, end - starts + 1, starts, end - starts);
	}

}
int main() {
	printf("The crawled URL is:");
	char url[128] = "http://www.netbian.com/";//http://www.netbian.com/ http://www.xinhuanet.com/ http://www.news.cn/politics/xxjxs/index.htm
	Spider sp;
	Spider_init(&sp, url);
	printf("Host:%s  resPath:%s\n", sp.host, sp.resPath);
	getHtml(&sp);
	return 0;
}

//获取网页
//char content[20] = { 0 };
//char res[1024] = { 0 };
//int len = recv(spider->fd, res, 1024, 0);
////int len = recv(spider->fd, html, 1024 * 100, 0);
//if (len == SOCKET_ERROR)
//{
//	printf("recv failed %d\n", WSAGetLastError());
//}
//char* ctbgin = strstr(res, "Content-Length: ");
//if (!ctbgin) {
//	return;
//}
//else {
//	printf("\n\n\n\n\n");
//	ctbgin += 15;
//}
//char* ctend = strstr(res, "Connection: ");
//if (!ctend) {
//	printf("网页错误\n");
//}
//else {
//	strncpy_s(content, ctend - ctbgin + 1, ctbgin, ctend - ctbgin);
//}
//int *cl = atoi(content);
//char html[105774] = {0};
//int i = 0;
//char ch=0;
//int len = recv(spider->fd, &ch, sizeof(ch), 0);
////int len = recv(spider->fd, html, 1024 * 100, 0);
//if (len == SOCKET_ERROR)
//{
//	printf("recv failed %d\n", WSAGetLastError());
//}
//printf("%c", ch);
//html[i++] = ch;
//while (recv(spider->fd, &ch, sizeof(ch), 0))
//{
//	printf("%c",ch);
//	html[i++] = ch;
//}
//FILE* fp;
//errno_t err = fopen_s(&fp, "maye.html", "w");
//if (!fp)
//	return;
//fwrite(html, sizeof(char), strlen(html), fp);
//fclose(fp);

//HRESULT ret = URLDownloadToFileA(NULL, imgUrl, "C:\\Users\\asus\\Desktop\\txt\\SPIDERs\\first.png", 0, NULL);//下载
	//if (ret == S_OK) {
	//	printf("\nThe image is downloaded successfully！ ");
	//}
	//else
	//{
	//	printf("\nThe image is downloaded failedly！ ");
	//}
	//printf("%s\n", imgUrl);