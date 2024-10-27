#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define HTTP_VERSION "HTTP/1.0"
#define HOST_OPT "Host"
// #define USER_AGENT "User-Agent"
#define CONNECTION_OPT "Connection"
#define CLOSE_STATUS "close"
#define PROXY_OPT "Proxy-Connection"
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

/**
 *   使用 fork 模式实现并发服务器
 */

/**
 *  这里定义需要使用的函数,buf时传出参数
 */

static void parseRequest(char* request , char* buf , char* host);  /*  用于解析 HTTP 请求,并且生成新的发送给服务器端的请求  */
static void send_request(char* request , char* host , char* response);
void child_handler(int sig);
int main(int argc , char* argv[])
{
    int listenfd , connfd;  /*  监听文件描述符,连接文件描述符   */
    // struct sockaddr_in client_addr;  /* 客户端地址结构 */
    char buf[1024] , client_buf[1024];  /* 需要使用的缓冲区 */
    char new_request[1024];  /* 需要发送的请求 */
    char host[100];  /* 主机名 */
    int count;  /*  用于 Read接受信息  */
    char response[30000];  /* 用于接受返回信息 */
    sigset_t sig;  /* 当前需要设置的信号集 */
    // sigset_t pre_sig;  /* 之前的信号集 */
    // socklen_t client_len = sizeof(client_addr);
    if(argc < 2) {
        app_error("please input an port ID to be listened by the proxy server!");
        exit(1);  
    }
    int res = atoi(argv[1]);  /* proxy监听的端口 */
    if(res == 0) {
        app_error("A vaild port ID!");
        exit(1);
    }
    // // 1. 获取 listenfd
    // listenfd = Socket(AF_INET , SOCK_STREAM , 0); 
    // // 2. 绑定端口和主机名
    // struct sockaddr_in server_addr;
    // server_addr.sin_port = htons(server_port);
    // server_addr.sin_family = AF_INET;
    // server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // Bind(listenfd , (struct sockaddr*)&server_addr , sizeof(server_addr));
    
    // // 3. Listen函数
    // Listen(listenfd , 1024);
    listenfd = Open_listenfd(argv[1]);
    int optval = 1;
    Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval , sizeof(optval));
    // 设置屏蔽
    // 注册信号屏蔽函数
    Signal(SIGCHLD , child_handler);
    // 4. 开始监听实践能力
 
    while(1) {
        connfd = Accept(listenfd , NULL , NULL);
        Sigemptyset(&sig);
        Sigaddset(&sig , SIGCHLD);
        Sigprocmask(SIG_BLOCK , &sig , NULL);
        if(Fork() == 0) {
            printf("I am a process[%d]\n" , getpid());
            // 子进程
            Close(listenfd);
            // 1. 读取信息,注意变量读时共享,写时复制
            count = Read(connfd , buf , sizeof(buf));
            if(count == 0) {
                app_error("Read to an end!");
            }
            // 2. 解析请求
            parseRequest(buf , new_request , host);
            // 3. 发送请求
            send_request(new_request , host , response);
            // 4. 写回给客户端
            count = write(connfd , response , strlen(response));
            if(count < 0) {
                app_error("write into client failed !");
            }
            // 5. 关闭
            // memset(buf , 0 , sizeof(buf));
            // memset(response , 0 , sizeof(response));
            // memset(host , 0 , sizeof(host));
            // memset(new_request , 0 , sizeof(new_request));
            // Close(connfd);
            exit(0);
        } else {
            // 父进程
            Close(connfd); // 这里可以直接关闭了,不会使得文件描述符在子进程中关闭
            // 解除屏蔽
            Sigemptyset(&sig);
            Sigaddset(&sig , SIGCHLD);
            Sigprocmask(SIG_UNBLOCK ,&sig , NULL);
        }
    }
    return 0;
}

/**
 *  1. 解析请求中的 资源路径和 主机名称以及端口
 *  2. 生成新的需要发送的 HTTP 请求并且返回
 */
static void parseRequest(char* request , char* buf , char* hostName) 
{   
    printf("%s" , request);
    char line[1024];  /* 读取一行 */
    char host[100];  /* 主机名称 */
    char resource[1024];  /* 资源路径 */
    char method[4];   /* 请求方式 */
    char version[20];  /* 请求版本 */
    int lineNum = 0;  /* 行号 */
    char option[20];  /* 请求头键 */
    char value[100];  /* 请求头值 */
    char resourceName[100];  /* 请求资源 */
    char addInfo[1024];  /* 请求头中的额外信息 */
    int i = 0;  // 记录索引
    char* cur = request;
    printf("\n");
    while(1) {
        if((*cur) == '\0') break;
        while((*cur) != '\r') {
            line[i] = *cur;
            cur ++;
            i ++;
        } 
        line[i + 1] = '\0';
        printf("%s\n",line);
        // 开始解析
        if(lineNum == 0) {
            sscanf(line , "%s %s %s" , method , resource , version);
            // 在对于 http 请求进行操作
            sscanf(resource , "%*[^/]\/\/%*[^/]%s" , resourceName);
        } else {
            sscanf(line , "%s %s" , option , value);
            if(strcmp(option , "Host:") == 0) strcpy(host , value);
            else if(strcmp(option , "User-Agent:") == 0);
            else if(strcmp(option , "Connection:") == 0);
            else if(strcmp(option , "Proxy-Connection:") == 0);
            else sprintf(addInfo , "%s%s\r\n" , addInfo , line);
        }
        while((*cur) == '\n' || (*cur) == '\r') {
            cur ++;
        }
        memset(line , 0 , sizeof(line));
        i = 0; 
        lineNum ++;
    }
    // 解析完成开始拼接新的 HTTP 请求
    char request_to_send[1024];
    sprintf(request_to_send , "%s %s %s\r\n" , method , resourceName , HTTP_VERSION); /* 请求行 */
    sprintf(request_to_send , "%s%s: %s\r\n" , request_to_send , HOST_OPT, host);
    sprintf(request_to_send , "%s%s" , request_to_send , user_agent_hdr);
    sprintf(request_to_send , "%s%s: %s\r\n" , request_to_send , CONNECTION_OPT , CLOSE_STATUS);
    // 拼接 addInfo
    sprintf(request_to_send , "%s%s" , request_to_send , addInfo);
    sprintf(request_to_send , "%s%s: %s\r\n\r\n" , request_to_send , PROXY_OPT , CLOSE_STATUS);

    printf("request to send: \n");
    printf("%s" , request_to_send);
    strcpy(buf , request_to_send);
    strcpy(hostName , host);
}

static void send_request(char* buf , char* host , char* response)
{
    // localhost:port
    char hostName[100];
    char port[20];
    int index = 0;
    while(host[index] != ':') {
        hostName[index] = host[index];
        index ++;
    }
    hostName[index] = '\0';
    index ++;
    int i = 0;
    while(host[index] != '\0') {
        port[i] = host[index];
        index ++;
        i ++;
    }
    port[i] = '\0';
    int clientfd;
    printf("hostName = %s , port = %s \n" , hostName , port);
    clientfd = Open_clientfd(hostName , port);
    // 直接发送信息
    Write(clientfd , buf , strlen(buf));
    // 读取信息
    char resp[30000];
    Read(clientfd , resp , sizeof(resp));
    strcpy(response , resp);
}

void child_handler(int sig)
{
    // 进行子进程的回收
    printf("enter into child_handler!\n");
    pid_t pid;
    while((pid = waitpid(-1 , NULL , WNOHANG)) > 0) {
        printf("resource pid : %d \n" , pid);
    }
}