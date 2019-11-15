#include <stdio.h>
#include "sock.h"
#include <string.h>
#include <time.h>

#define RESPATH_MAX_LENGTH 512
#define CACHE_MAX 1448
#define FULL_PATH_MAX_LENGTH 1024
#define CONTENT_TYPE_COUNT 22

S_SOCKADDR  global_sockaddr;
int         global_timeout;
char        global_respath[RESPATH_MAX_LENGTH];

void print_usage(char* cmd) // print help and usage message
{
	printf("\
Simple HTTP Server v1.0\n\
Usage: %s ip:port [-t timeout] [-p respath]\n\
	ip       server ip address\n\
	port     server port\n\
	-t       waiting time when recving data\n\
	timeout  number bigger than 0\n\
	-p       resource file(s) path\n\
	respath  a valid local path no more than 512 bytes\n\
help:\n\
    -h       show help message\n\
", cmd);
}

void print_ip(unsigned long ip) // print network ip address in dec
{
    ip = sock_ntohl(ip);
    printf("%u.", ip >> 24);
    ip = ip << 8;
    printf("%u.", ip >> 24);
    ip = ip << 8;
    printf("%u.", ip >> 24);
    ip = ip << 8;
    printf("%u", ip >> 24);
}

void print_host(unsigned long ip, unsigned short port)//print network port
{
    ip = sock_ntohl(ip);
    printf("%u.", ip >> 24);
    ip = ip << 8;
    printf("%u.", ip >> 24);
    ip = ip << 8;
    printf("%u.", ip >> 24);
    ip = ip << 8;
    printf("%u:", ip >> 24);
    printf("%u", sock_ntohs(port));
}

unsigned int str_to_int(char* in) //parse string to unsigned integer
{
	int i;
	unsigned int result = 0;
	
	for(i = 0; in[i] != '\0'; i++)
	{
		if(in[i] < '0' || in[i] > '9')
			break;
        result = result * 10;
        result += in[i] - '0';
	}
	return result;
}

int int_to_str(unsigned int in, char* out) // translate unsigned int to string
{
    int digit = 0, i;
    unsigned int itmp;

    if (in == 0)
    {
        out[0] = '0';
        out[1] = '\0';
        return 1;
    }
    itmp = in;
    while (itmp > 0)
    {
        digit++;
        itmp = itmp / 10;
    }
    out[digit] = '\0';
    itmp = in;
    for (i = digit - 1; i >= 0; i--)
    {
        out[i] = itmp % 10 + '0';
        itmp = itmp / 10;
    }
    return digit;
}

int read_args(int argc, char** argv) //read command arguments
{
	int a, i, j, ret;
	int port = 0, timeout = 0;
	char cip[16];
	
	ret = 0;
	if(argc < 1)
		return -1;
	else if(argc < 2)// command too little
	{
		print_usage(argv[0]);
		return -1;
	}
    else if (argv[1][0] == '-' && argv[1][1] == 'h')//help command
    {
        print_usage(argv[0]);
        return -1;
    }
	// argv 1, ip and port
	a = 0;
	for(i = 0; argv[1][i] != '\0' && argv[1][i] != ':'; i++)
    {
        if (argv[1][i] == '.') //to judge if the nomber of points is three
            a++;
		else if(argv[1][i] < '0' || argv[1][i] > '9') //to judge if all digits
			break;
	}
	if(i > 3 && i < 16 && a == 3) //to juege if the length is right
	{
		a = i;
		//ip
		for(i = 0; i < a; i++) //copy to temp
			cip[i] = argv[1][i];
	    cip[i] = '\0';
	    global_sockaddr.ip = sock_inet_addr(cip);//set ip address
		//port
	    port = str_to_int(argv[1]+i+1);//translate to integer
        if (port < 0 || port > 65535)//judge port
        {
            ret = -1;
            printf("port number invalid.\n");
        }
        else
			global_sockaddr.port = sock_htons((short)port);//set port
	}
    else
    {
        ret = -1;
        printf("ip number invalid.\n");
    }
	//others
	for(a = 2; a < argc; a++)
	{
		if(argv[a][0] != '-')
		{
			printf("unknown argument %s\n", argv[a]);
			ret = -1;
			break;
		}
		if(argv[a][1] == 't') //command -t
		{
            a++;
			timeout = str_to_int(argv[a]);
			if(timeout >= 0)
				global_timeout = timeout;
		}
		else if(argv[a][1] == 'p')//command -p
		{
            a++;
			for(i = 0, j = 0; argv[a][i] != '\0' && j < RESPATH_MAX_LENGTH; i++)
			{
                if (argv[a][i] == '\\')//change '\' to '/'
                {
                    global_respath[j] = '/';
                    j++;
                }
                else if (argv[a][i] != '"')//ignore '"'
                {
                    global_respath[j] = argv[a][i];
                    j++;
                }
			}
            if (j > 0 && global_respath[j - 1] != '/')//add '/' at end if necessory
            {
                global_respath[j] = '/';
                j++;
            }
			global_respath[j] = '\0';
		}//end if
		else
		{
			printf("unknown argument %c%c", argv[a][0], argv[a][1]);
			ret = -1;
		    break;
		}
	}//end for
	if(ret < 0)
		print_usage(argv[0]);
	return ret;
}

int analyze_header(char* buf, int length, char* path, int* path_ind1)//analyze request header
{
    int i, k;
    char ctmp[16];
    for (i = 0; i < length && i < 15 && buf[i] != ' '; i++)
        ctmp[i] = buf[i];
    ctmp[i] = '\0';
    //request method
    if (strcmp(ctmp, "GET") != 0)
        return 501;// not supported
    for (; i < length && buf[i] != '/'; i++) ;
    //res path
    for (k = 0; k < RESPATH_MAX_LENGTH && global_respath[k] != '\0'; k++)//copy res path
        path[k] = global_respath[k];
    *path_ind1 = k;
    //full path
    for (i++; i < length; i++)
    {
        if (buf[i] == ' ' || buf[i] == '?' || buf[i] == '#' || buf[i] == '\r' || buf[i] == '\n')
            break;
        //handle invalid path
        if (buf[i] == '\\')
            buf[i] = '/';
        if (buf[i] == '/' && i+1 < length)// case '/////...' '/..' '/.'
        {
            if (buf[i+1] == '\0' || buf[i+1] == '/')
                return 400;
            else if (buf[i + 1] == '.' && i + 2 < length)
            {
                if (buf[i + 2] == '\\' || buf[i + 2] == '/' || buf[i + 2] == '.')
                    return 400;
            }
        }
        else if (buf[i] == '.' && i + 1 < length)
        {
            if (buf[i + 1] == '\\' || buf[i + 1] == '/' || buf[i + 1] == '.')
                return 400;
        }
        ////
        path[k] = buf[i];
        k++;
    }
    path[k] = '\0';
    //http version
    for (; i < length && buf[i] != ' '; i++) ;
    for (i++, k = 0; i < length && buf[i] != '/'; i++, k++)
        ctmp[k] = buf[i];
    ctmp[k] = '\0';
    if (strcmp(ctmp, "HTTP") != 0)
        return 400;
    return 200;
}

const char *wday[8] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "GMT" };
const char *month[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

int getdate(char* in)
{
    int i, k, n;
    time_t ltime;
    struct tm ftime;

    time(&ltime);
    localtime_s(&ftime, &ltime);
    // Date: Sat, 05 Oct 2019 09:09:40 GMT
    n = 0;
    //week day
    k = ftime.tm_wday;
    for (n = 0; wday[k][n] != '\0'; n++)
        in[n] = wday[k][n];
    in[n] = ',';
    in[n + 1] = ' ';
    n += 2;
    //day
    in[n] = ftime.tm_mday / 10 + '0';
    in[n+1] = ftime.tm_mday % 10 + '0';
    in[n + 2] = ' ';
    n += 3;
    //month
    k = ftime.tm_mon;
    for (i = 0; month[k][i] != '\0'; i++)
        in[n + i] = month[k][i];
    in[n + i] = ' ';
    n += i + 1;
    //year
    k = ftime.tm_year + 1900;
    in[n] = k / 1000 + '0';
    k = k % 1000;
    in[n+1] = k / 100 + '0';
    k = k % 100;
    in[n+2] = k / 10 + '0';
    in[n+3] = k % 10 + '0';
    in[n + 4] = ' ';
    n += 5;
    //time
    in[n] = ftime.tm_hour / 10 + '0';
    in[n+1] = ftime.tm_hour % 10 + '0';
    in[n+2] = ':';
    in[n+3] = ftime.tm_hour / 10 + '0';
    in[n+4] = ftime.tm_hour / 10 + '0';
    in[n+5] = ':';
    in[n+6] = ftime.tm_hour / 10 + '0';
    in[n+7] = ftime.tm_hour / 10 + '0';
    in[n + 8] = ' ';
    n += 9;
    //gmt
    in[n]   = 'G';
    in[n+1] = 'M';
    in[n+2] = 'T';

    in[n + 3] = '\0';
    return n+3;
}

const char *fileType[CONTENT_TYPE_COUNT] = { "text/html", "text/html", "application/javascript",
    "text/css", "img/jpeg", "img/jpeg", "image/jpeg", "image/png", "image/gif", "text/xml", "text/txt", "audio/mp3",
    " image/x-icon", "audio/wav", "application/zip", "audio/mp4", "audio/mid", "	message/rfc822",
    "text/xml", "application/json", "application/x-shockwave-flash", "application/octet-stream" };
const char *fileEnd[CONTENT_TYPE_COUNT] = { "htm", "html", "js",
    "css", "jpg", "jpe", "jpeg", "png", "gif", "xml", "txt", "mp3",
    "ico", "wav", "zip", "mp4", "mid", "eml", "svg", "json", "swf", "*" };

int find_contenttype(const char* in)//judge content type by suffix name of file 
{
    int i, j;
    for (i = 0; i < CONTENT_TYPE_COUNT - 1; i++)
    {
        for (j = 0; in[j] != '\0' && fileEnd[i][j] != '\0'; j++)//string compare
        {
            if (fileEnd[i][j] >= 'a' && fileEnd[i][j] <= 'z')//characher, ignore upper or lower
            {
                if (in[j] != fileEnd[i][j] && (in[j]^32) != fileEnd[i][j])
                    break;
            }
            else
            {
                if (in[j] != fileEnd[i][j])
                    break;
            }
        }
        if (in[j] == '\0' && fileEnd[i][j] == '\0')
            break;
    }
    return i;
}

const char* s200 = "OK";
const char* s400 = "Bad Request";
const char* s404 = "Not Found";
const char* s501 = "Not Implemented";

const char* s_server = "Server: Simple HTTP Server";
const char* s_connection = "Connection: ";
const char* s_connclose = "close";
const char* s_connka = "keep-alive";
const char* s_date = "Date: ";
const char* s_content_type = "Content-type: ";
const char* s_content_length = "Content-Length: ";


int makeheader(char* path, int path_ind1,int status, char* buf, FILE** ppfile)//make response header
{
    FILE *pfile = NULL;
    int i, n, m, length;
    const char *pct;
    char ctmp[24];
    //http version
    strcpy(buf, "HTTP/1.1");
    length = 8;
    buf[length] = ' ';
    length++;
    //file test
    if (status == 200)
    {
        pfile = fopen(path, "rb");
        if (pfile == NULL)
            status = 404;
    }
    //response code
    n = int_to_str(status, ctmp);
    for (i = 0; i < n; i++)
        buf[length + i] = ctmp[i];
    buf[length + i] = ' ';
    length = length + i + 1;
    //status
    switch (status)
    {
    case 400:
        pct = s400;
        break;
    case 404:
        pct = s404;
        break;
    case 501:
        pct = s501;
        break;
    default:
        pct = s200;
        break;
    }
    for (i = 0; pct[i] != '\0'; i++)
        buf[length + i] = pct[i];
    length += i;
    //next line
    buf[length] = '\r';
    buf[length + 1] = '\n';
    length += 2;
    //server
    for (i = 0; s_server[i] != '\0'; i++)
        buf[length + i] = s_server[i];
    length += i;
    //next line
    buf[length] = '\r';
    buf[length + 1] = '\n';
    length += 2;
    //connection
    for (i = 0; s_connection[i] != '\0'; i++)
        buf[length + i] = s_connection[i];
    length += i;
    //close
    for (i = 0; s_connclose[i] != '\0'; i++)
        buf[length + i] = s_connclose[i];
    length += i;
    //next line
    buf[length] = '\r';
    buf[length + 1] = '\n';
    length += 2;
    //date
    for (i = 0; s_date[i] != '\0'; i++)
        buf[length + i] = s_date[i];
    length += i;
    i = getdate(buf + length);
    length += i;//next line
    buf[length] = '\r';
    buf[length + 1] = '\n';
    length += 2;
    //content type
    for (n = path_ind1; path[n] != '\0'; n++) ;
    for (n--; n > path_ind1 && path[n] != '.'; n--);
    if (status != 200)
        n = 0;
    else if (n <= path_ind1 || path[n] == '\0')
        n = CONTENT_TYPE_COUNT - 1;
    else
        n = find_contenttype(path+n+1);
    for (i = 0; s_content_type[i] != '\0'; i++)
        buf[length + i] = s_content_type[i];
    length += i;
    for (i = 0; fileType[n][i] != '\0'; i++)
        buf[length + i] = fileType[n][i];
    length += i;
    //next line
    buf[length] = '\r';
    buf[length + 1] = '\n';
    length += 2;
    //content length
    if (pfile != NULL)
    {
        fseek(pfile, 0, SEEK_END);
        m = ftell(pfile);
        fseek(pfile, 0, SEEK_SET);
        n = m - ftell(pfile);
    }
    else
        n = 0;
    m = int_to_str(n, ctmp);
    for (i = 0; s_content_length[i] != '\0'; i++)
        buf[length + i] = s_content_length[i];
    length += i;
    for (i = 0; i < m; i++)
        buf[length + i] = ctmp[i];
    length += i;
    //next line
    buf[length] = '\r';
    buf[length + 1] = '\n';
    length += 2;
    //next line
    buf[length] = '\r';
    buf[length + 1] = '\n';
    length += 2;
    buf[length] = '\0';

    *ppfile = pfile;
    return length;
}

int main(int argc, char** argv)
{
    //变量 variable
    S_SOCKET servsocket;
    S_SOCKET clntsocket;
    S_SOCKADDR clntaddr;
    //int ret;
    int status, ind1;
    int cache_length;
    FILE *pfile;
    char fullpath[FULL_PATH_MAX_LENGTH];
    char cache[CACHE_MAX];
    //初始化变量 initinalize variable
    //global_sockaddr.family = AF_INET;
    global_sockaddr.ip = 0;
    global_sockaddr.port = 80;
    global_timeout = 500;
    global_respath[0] = '\0';
    //读取参数 read command arguments
	if(read_args(argc, argv) < 0)
		return -1;
    /*
    int test_argc = 3;
    char *test_argv[3] = { "server", "0.0.0.0:80", "-p:.\\web\\" };
    if (read_args(test_argc, test_argv) < 0)
        	return -1;
    */
    //回显参数 echo arguments
    printf("IP: ");
    print_ip(global_sockaddr.ip);
    printf("PORT: %u\n", sock_ntohs(global_sockaddr.port));
    printf("TIMEOUT: %d\n", global_timeout);
    printf("RESOURCE PATH: %s\n", global_respath);
	//初始化 initinalize socket
    sock_init();
	//创建socket create socket
    servsocket = sock_socket_tcp_stream();
    if (servsocket == NULL)
    {
        printf("socket error!\n");
        sock_finalize();
        return -1;
    }
	//绑定地址 bind address and port
    if (sock_bind(servsocket, &global_sockaddr) < 0)
    {
        printf("bind error!\n");
        sock_finalize();
        return -1;
    }
	//监听 listen
    sock_listen(servsocket, 20);

	while(1)
	{
        //初始化 initinalize
        fullpath[0] = '\0';
        clntsocket = NULL;
        pfile = NULL;
		//接收连接 accept connection
        clntsocket = sock_accept(servsocket, &clntaddr);
        if (clntsocket == NULL)
            continue;
        //日志输出 log out
        printf("connection %u | ", clntsocket);
        print_host(clntaddr.ip, clntaddr.port);
        putchar(' ');
        //设置连接 set connection timeout
        if (global_timeout > 0)
        {
            if (sock_set_timeout(clntsocket, global_timeout) < 0)
            {
                sock_closesocket(clntsocket);
                printf("| error when setting timeout value.\n");
                continue;
            }
        }
		//读取 read request
        cache_length = sock_recv(clntsocket, cache, CACHE_MAX, 0);
        if (cache_length <= 0)
        {
            sock_closesocket(clntsocket);
            printf("| bad connection.\n");
            continue;
        }
		//分析请求头 analyze request
        status = analyze_header(cache, cache_length, fullpath, &ind1);
		//生成响应头 make response
        pfile = NULL;
        cache_length = makeheader(fullpath, ind1, status, cache, &pfile);
		//发送响应头 send response header
        sock_send(clntsocket, cache, cache_length, 0);
		//发送文件 send file
        if (pfile != NULL)
        {
            fseek(pfile, 0, SEEK_SET);
            cache_length = CACHE_MAX;
            while (cache_length >= CACHE_MAX && (!feof(pfile)))
            {
                cache_length = fread(cache, sizeof(char), CACHE_MAX, pfile);
                sock_send(clntsocket, cache, cache_length, 0);
            }
            fclose(pfile);
        }
        //close
        sock_closesocket(clntsocket);
        //日志输出 log out
        printf("| OK\n");
	}
	
    sock_finalize();
	return 0;
}

