#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define MAXLINE 100  /* 表示读取一行的最大的字符数量 */
#define MAX_THREA_NUM 100  /* 最大线程数量 */
#define MAX_BUFFER_SIZE 1024
/*************************
 *  利用多线程实现并发服务器,这里利用信号量实现生产者消费者模型
 *  大致思路:
 *      利用一个缓冲区 buf,其中存储需要处理的 connfd
 *      每一个线程的任务就不断循环从缓冲区中取出 connfd 并且调用doit进行通信即可
 **************************/
typedef struct Sbuf_t {
    int* buffer;  /* 文件描述符号缓冲区 */
    int n;  /* 缓冲区中的最大元素数量 */
    int front;  /* 环形队列前面的指针 */
    int rear;  /* 环形队列最后一个指针 */
    pthread_mutex_t mutex;  /* 互斥信号量,保护缓冲区*/
    sem_t slots;  /* 空位数量 */
    sem_t items;  /* 文件描述符号个数 */

} sbuf_t;

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

/* 线程执行函数  */
static void* thread_handler(void* arg);  // arg可以传递 connfd,利用值传递的方式即可

/* 以下定义一系列与缓冲区操作有关的函数 */
static void sbuf_init(sbuf_t* sbuf , int n); /* 初始化 */
static void sbuf_destory(sbuf_t* sbuf);  /* 销毁sbuf */
static void sbuf_insert(sbuf_t* sbuf , int fd);  /* 向缓冲区中加入元素 */
static int sbuf_remove(sbuf_t* sbuf);

sbuf_t sbuf;  /* 全局变量 */
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
    // 初始化相关变量和线程
    sbuf_init(&sbuf , MAX_BUFFER_SIZE);
    // 初始化线程
    pthread_t tid;
    for(int i = 0 ; i < MAX_THREA_NUM ; i ++) {
        pthread_create(&tid , NULL , thread_handler , NULL);
    }
    // 3. 获取到 connfd
    while(1) {
        connfd = Accept(listenfd , NULL , NULL);
        // 开始解析信息
        sbuf_insert(&sbuf , connfd);
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

static void sbuf_init(sbuf_t* sbuf , int n)
{
    sbuf -> buffer = Calloc(n , sizeof(int));
    sbuf -> n = n;
    sbuf -> front = sbuf -> rear = 0;  /* 此时可以利用信号量表示队列的空和满 */
    Sem_init(&(sbuf -> slots) , 0 ,n);  /* 0 表示线程之间同步 */
    Sem_init(&(sbuf -> items) , 0 ,0);
    pthread_mutex_init(&(sbuf->mutex) , NULL);

}

static void sbuf_destory(sbuf_t* sbuf)
{
    Free(sbuf -> buffer);
    pthread_mutex_destroy(&(sbuf -> mutex));
}

static void sbuf_insert(sbuf_t* sbuf , int fd)
{
    // 需要通过 PV 操作
    P(&(sbuf->slots));
    pthread_mutex_lock(&(sbuf->mutex));
    sbuf->rear = (sbuf->rear + 1) % (sbuf -> n);
    sbuf->buffer[sbuf->rear] = fd;
    pthread_mutex_unlock(&(sbuf->mutex));
    V(&(sbuf->items));  
}

static int sbuf_remove(sbuf_t* sbuf)
{
    // 从队列中移除元素
    int res;
    P(&(sbuf->items));
    pthread_mutex_lock(&(sbuf->mutex));
    sbuf->front = (sbuf->front + 1) % (sbuf->n);
    res = sbuf->buffer[sbuf->front];  /* 这里 front 指向第一个元素前面的一个位置 */
    pthread_mutex_unlock(&(sbuf->mutex));
    V(&(sbuf->slots));
    return res;
}
/* 定义线程行为 */
void* thread_handler(void* arg)
{
    // 线程的行为就是不断从缓冲区中取出元素
    pthread_detach(pthread_self());
    while(1) {
        int connfd = sbuf_remove(&sbuf);
        doit(connfd);
        Close(connfd);
        // 这里还可以设置退出点退出程序
    }
}