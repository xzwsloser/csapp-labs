#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define READ_BUF_SIZE 1024  /* 读写缓冲区的长度 */

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
/* 请求结构体 */
typedef struct Request {
    char method[4];  /* 请求方式 */
    char URL[100];  /* 请求的 URL */
    char resource[100];  /* 请求资源 */
    char version[20];  /* HTTP 版本 */
    char host[100];  /* 主机名称:端口号 */
    char hostName[100];  /* 主机名称 */
    char port[10];  /* 端口号 */
    char addInfo[1024];  /* 表示其他的字段 */
} Request;

/* 需要发送的请求结构体 */
typedef struct Request_to_send {
    char hostName[100];  /* 主机名 */
    char port[10];   /* 端口号 */
    char request[1024];   /* 请求体 */
} Request_to_send;

/* 接收到的响应 */
typedef struct Response {
    char response[30000];  /* 响应 */
} Response;
/**
 *   顺序结构实现的 Proxy 服务器
 */

/** 
 *  改为多线程版本的
 */
static void parse_request(char* read_buf , Request_to_send* request);
static void send_request(Request_to_send* request , Response* response);

int main(int argc , char* argv[])
{
    if(argc < 2) {
        app_error("please input a port!");
        exit(1);
    }
    // 1. 首先获取 listenfd;
    int listenfd , connfd;
    // rio_t rio;  /* 缓冲区 */
    char read_buf[READ_BUF_SIZE];  /* 用户缓冲区 */
    Request_to_send request;   /* 需要发送的请求 */
    Response resp;  /* 响应 */
    listenfd = Open_listenfd(argv[1]);
    // 设置端口复用
    int optval = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,    //line:netp:csapp:setsockopt
                   (const void *)&optval , sizeof(int));
    Signal(SIGPIPE , SIG_IGN);
    // 2. 监听操作
    while(1) {
        connfd = Accept(listenfd , NULL , NULL);
        // 3. 首先读读取请求
        Read(connfd , read_buf , sizeof(read_buf)); /* 只用读取一次 */
        // 4. 开始解析请求并且生成对应的请求
        parse_request(read_buf , &request);
        // 5. 发送请求并且得到响应
        send_request(&request , &resp);
        // printf("%s" , resp.response);
        // 6. 请求写回给客户端
        Write(connfd , (resp.response) , strlen(resp.response));
        memset(&request , 0 , sizeof(request));
        memset(&resp , 0 , sizeof(resp));
        memset(read_buf , 0 , sizeof(read_buf));
        // sleep(1);
        Close(connfd);
    }
    return 0;
}

/**
 *   读取请求并且重新组装请求,完成需要发送的请求
 */
static void parse_request(char* read_buf , Request_to_send* request)
{
    printf("==========\n");
    printf("%s" , read_buf);
    printf("==========\n");
    Request req;
    // 1. 开始解析请求
    char line[1024];  /* 解析每一行 */
    int lineNum = 0;  /* 表示行号  */
    int index = 0;    /* 表示当前索引 */  
    char opt[100];  /* 表示当前请求头的选项 */
    char value[100];  /* 当前请求头的值 */
    char* cur = read_buf;
    memset(line , 0 , sizeof(line));
    memset(req.addInfo,0 , sizeof(req.addInfo));
    memset(request->request , 0 , sizeof(request->request));
    while((*cur) != '\0') {
        // 开始读取
        if((*cur) == '\0') break;
        while((*cur) != '\r') {
            line[index] = (*cur);
            cur ++;
            index ++;
        }
        line[index] = '\0';
        // 开始解析
        if(lineNum == 0) {
            // 表示第一行
            // GET http://localhost:1000/home.html HTTP/1.1
            sscanf(line , "%s %s %s" , req.method , req.URL , req.version);
            // 解析 URL
            sscanf(req.URL , "%*[^/]\/\/%*[^/]%s" , req.resource);
        } else {
            printf("line =%s\n" , line);
            sscanf(line , "%[^:]: %s" , opt , value);
            printf("opt =%s, value=%s\n" , opt , value);
            if(strcmp(opt , "Host") == 0) {
                // 解析 host和hostName还有 port
                strcpy(req.host , value);
                // 解析 hostName 和 port
                sscanf(req.host , "%[^:]: %s" , req.hostName , req.port);
            } else if(strcmp(opt , "User-Agent") == 0) {

            } else if(strcmp(opt , "Connection") == 0) {

            }  else if(strcmp(opt , "Proxy-Connection") == 0) {

            }
            else {
                // 进行拼接
                sprintf(req.addInfo , "%s%s\r\n" , req.addInfo ,line);
            }
        }
        // 进行下一步的匹配
        cur ++;
        while((*cur) == '\n') {
            cur ++;
        }
        if((*cur) == '\r' && *(cur+1) == '\n') break;
        index = 0;
        lineNum++;
        memset(line , 0 , sizeof(line));
        memset(opt , 0 , sizeof(opt));
        memset(value , 0 , sizeof(value));
    }
    // 2. 解析完毕可以发送请求了
    strcpy(request -> hostName , req.hostName);
    strcpy(request -> port , req.port);
    req.addInfo[strlen(req.addInfo)] = '\0';
    // 3. 拼接请求
    char new_request[1024];
    sprintf(new_request , "%s %s %s\r\n" , req.method , req.resource , "HTTP/1.1");
    sprintf(new_request , "%s%s: %s\r\n" , new_request , "Host" , req.host);
    sprintf(new_request , "%s%s" , new_request , user_agent_hdr);
    sprintf(new_request , "%s%s: %s\r\n" , new_request , "Connection" , "close");
    sprintf(new_request , "%s%s" , new_request , req.addInfo);
    sprintf(new_request , "%s%s: %s\r\n\r\n" , new_request , "Proxy-Connection" , "close");
    // 拼接其他请求
    printf("%s" , req.addInfo);
    printf("===================\n");
    new_request[strlen(new_request)] = '\0';
    strcpy(request -> request , new_request);
    printf("%s" , request->request);
}


static void send_request(Request_to_send* request , Response* response)
{
    // 1. 首先获取 Clientfd
    int clientfd;
    int count;
    clientfd = Open_clientfd(request -> hostName , request -> port);
    // 2. 发送请求
    Write(clientfd , request->request , strlen(request->request));
    // 读取响应
    count = Read(clientfd , response->response , sizeof(response->response));
    if(count == 0) {
        app_error("Read a FIN!");
    }
}
