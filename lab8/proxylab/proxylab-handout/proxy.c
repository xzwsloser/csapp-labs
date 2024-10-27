#include <stdio.h>
#include "csapp.h"
#include<pthread.h>
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define MAXLINE 100  /* 表示读取一行的最大的字符数量 */
#define MAX_THREA_NUM 100  /* 最大线程数量 */
#define MAX_BUFFER_SIZE 1024
/*************************************************
 *  利用 LRU 缓存为代理服务器设置一层缓存
 *  这里对于缓存的读写需要使用读写锁,其实最好还是上hashmap,但是我这里就使用链表了
 *************************************************/

typedef struct Node {
    char* cache;  /* 缓存 */
    char* resourceName;  /* 资源名称 */
    int T;  /* 优先级 */
    int size;  /* 缓存大小 */
    struct Node* next;
} Node;

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

/* 与缓存操作相关的函数 */
static void cache_init(); /* node 时头节点 */
static char* get_cache(char* resourceName);  /* 查询缓冲中的数据并且返回 */
static void insert_cache(char* resp , char* resourceName);  /* 把 resp 存储在node中 */
static void eviction_cache(int size);  /* 进行空间的不断驱逐 */
sbuf_t sbuf;  /* 全局变量 */
Node* cache;  /* 缓存链表的头节点 */
int T;  /* 全局的时间表,用于标记访问的次数 */
pthread_mutex_t mutex; /* 用于保护全局变量 T */
pthread_rwlock_t rwlock;  /* 用于保护共享的缓冲区 */
int sumSize;  /* 缓存的总大小 */
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
    // 初始化锁和缓冲区
    cache_init();
    T = 0;
    pthread_mutex_init(&mutex , NULL);
    pthread_rwlock_init(&rwlock , NULL);
    sumSize = 0; 
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
    // 可以直接进行搜索并且返回
    printf("Before get cache!\n");
    char* resp_cache = get_cache(url.resource);
    printf("After Cache!\n");
    if(resp_cache != NULL) {
        // 可以直接返回了
        printf("%s\n" , resp_cache);
        Rio_writen(fd , resp_cache , strlen(resp_cache));
        return;
    }
    printf("There is no cache!\n");
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
    char resp_to_cache[30000];
    char resp[200];
    while((n = Rio_readlineb(&server_rio , resp , MAXLINE)) != 0) {
        Rio_writen(connfd , resp , n);
        strcat(resp_to_cache , resp);
    }
    insert_cache(resp_to_cache , url->resource);
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

/* 初始化链表的头节点 */
static void cache_init()
{
    cache = (Node*)malloc(sizeof(Node));
    cache->resourceName=NULL;
    cache->cache=NULL;
    cache->size=0;
    cache->T=0;   
    cache->next=NULL;
}

/* 查询缓存中的数据并且返回,注意需要更新全局时间,这里时读取操作,但是这里还需要更新T,如何用读写锁 */
/* 这里获取缓存的时候还需要更新缓存,所以还有写操作*/
static char* get_cache(char* resourceName)
{
    char* ptr;
    pthread_rwlock_rdlock(&rwlock);
    Node* cur = cache -> next;
    while(cur != NULL) {
        if(strcmp(cur -> resourceName , resourceName) == 0) {
            ptr = cur->cache;
            break;
        }
        cur = cur -> next;
    }
    pthread_rwlock_unlock(&rwlock);
    printf("====cur====\n");
    if(cur == NULL) return NULL;
    printf("===cur is not null====\n");
    // 开始更新
    pthread_mutex_lock(&mutex);
    T++;
    cur->T = T;
    pthread_mutex_unlock(&mutex);
    return ptr;  /* 注意此时返回的就是一个堆区的地址 */
}

static void insert_cache(char* resp , char* resourceName)
{
    int n = strlen(resp);
    if(n >= MAX_OBJECT_SIZE) {
        return ;
    } else if(n + sumSize > MAX_CACHE_SIZE) {
        eviction_cache(n);
    }
    // 构造新的节点
    Node* pnew = (Node*)malloc(sizeof(Node));
    pnew -> size = n;
    pnew -> cache = (char*)malloc(sizeof(char)*n);
    strcpy(pnew->cache , resp);
    pnew->resourceName = (char*)malloc(sizeof(char)*strlen(resourceName));
    strcpy(pnew->resourceName , resourceName);

    pthread_mutex_lock(&mutex);
    T++;
    pnew->T = T;
    pthread_mutex_unlock(&mutex);

    pthread_rwlock_wrlock(&rwlock);
    // 利用头插法进行节点的插入
    pnew -> next = cache -> next;
    cache -> next = pnew;
    sumSize += pnew->size;
    pthread_rwlock_unlock(&rwlock);
}

/*
    驱逐出可以空出来 size 空间的缓存
*/
static void eviction_cache(int size)
{
    pthread_rwlock_wrlock(&rwlock);
    Node* cur = cache;
    while(cur->next != NULL && (sumSize + size > MAX_CACHE_SIZE)) {
        cur = cache;
        int minT = cur->next->T;
        Node* target;
        while(cur -> next != NULL) {
            if(cur->next->T <= minT) {
                minT = cur->next->T;
                target = cur;  // target->next需要删除
            }
        }
        sumSize -= target->next->size;
        Node* temp = target->next;
        target->next = temp -> next;
        free(temp);  // 释放空间
    }
    pthread_rwlock_unlock(&rwlock);
}