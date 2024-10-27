#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define MAXLINE 100  /* 表示读取一行的最大的字符数量 */
typedef struct URL {
    char host[100];  /* 主机名:端口号 */
    char hostName[100];  /* 主机名 */
    char port[10];   /* 端口号 */
    char resource[100];  /* 资源路径 */
} URL;


typedef struct Request_Header {
    char method[10];  /* 请求方式 */
    char uri[100];  /* 主机名称 */
    char version[20]; /* 版本号 */
} Request_Header;
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
/*  用于读取,获取主机名称等信息并且构建请求体 */
static void doit(int fd);
/* 用于解析 url 获得 URL 结构体*/
static void parse_url(char url[100] , URL* target);
/* 建立 Http 请求 */
static void build_request(URL* url ,char* request);
/* 发送请求到 Tiny 服务器 */
static void send_request(int fd , URL* url , char* request);
int main(int argc , char* argv[])
{
    if(argc < 2) {
        app_error("please input an port !\n");
        exit(1);
    }
    // 1. 设置信号阻塞,防止通道过早关闭
    Signal(SIGPIPE , SIG_IGN);
    // 2. 获取 listenfd
    int listenfd , connfd;
    listenfd = Open_listenfd(argv[1]);
    int optval = 1;
    Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval , sizeof(optval));
    // 3. 获取到 connfd
    while(1) {
        connfd = Accept(listenfd , NULL , NULL);
        // 开始解析信息
        doit(connfd);
        Close(connfd);
    }
    return 0;
}
/**
 *   fd: 需要读取的文件描述符
 *   request: 传出参数,用于接受最后得到的请求
 */
static void doit(int fd)
{   
    // 首行: GET http://localhost:3306/home.html HTTP/1.1
    char request[1024];
    Request_Header rh;
    URL url;
    char header[100];  /* 请求头 */
    rio_t rp;  /* 用于读取 */
    char resp[30000];  /* 响应 */
    Rio_readinitb(&rp , fd);
    Rio_readlineb(&rp , header , MAXLINE);
    // 1. 解析请求头
    // method = GET  uri = http://localhost:3306/home.html version=HTTP/1.1
    sscanf(header , "%s %s %s" , rh.method , rh.uri , rh.version);
    // 2. 解析 uri
    parse_url(rh.uri,&url);
    // 3. 拼接得到 http 请求
    build_request(&url , request);
    // 4. 发送请求
    send_request(fd , &url , request);
}


static void parse_url(char* url , URL* target)
{
    // url: http://localhost:3306/home.html
    // %*[^/]\/\/%[^/]%s
    // 解析 host 和 resource
    sscanf(url , "%*[^/]\/\/%[^/]%s" , target->host , target->resource);
    printf("host = %s , resource = %s\n" , target->host , target->resource);
    // 解析 hostName , port
    // %[^:]: %s
    sscanf(target->host , "%[^:]: %s" , target->hostName , target->port);
    printf("hostName = %s , port = %s\n" , target->hostName , target->port);
}
/* 获取 request */
static void build_request(URL* url , char* request)
{
    // url: host hostName port , resource
    printf("resource = %s\n" , url->resource);
    char req[1024];
    sprintf(req , "%s %s %s\r\n" , "GET" , url->resource , "HTTP/1.1"); /* 请求行 */  
    sprintf(req , "%s%s: %s\r\n" , req , "Host" , url->host);  
    sprintf(req , "%s%s: %s\r\n" , req , "Connection" , "close");
    sprintf(req , "%s%s: %s\r\n" , req , "Proxy-Connection" , "close");
    sprintf(req , "%s%s\r\n" , req , user_agent_hdr);
    int index = 0;
    strcpy(request , req);
    printf("%s" , request);
}

static void send_request(int connfd ,URL* url , char* request)
{
    // request: url req
    // 1. 获取到 clientfd
    int clientfd;
    clientfd = Open_clientfd(url->hostName , url->port);
    // 2. 写到客户端
    Rio_writen(clientfd , request , strlen(request));
    // 3. 读取响应
    rio_t server_rio;
    Rio_readinitb(&server_rio , clientfd);
    int n;
    char resp[200];
    while((n = Rio_readlineb(&server_rio , resp , MAXLINE)) != 0) {
        Rio_writen(connfd , resp , n);
    }
    close(clientfd);
}