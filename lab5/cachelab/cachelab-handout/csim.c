#include "cachelab.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#define BASE_NUM 48
typedef __uint64_t uint64_t;
// 定义一系列的全局变量用于记录状态
int result[3];  // 分别用于记录 hit_count , miss_count 和 eviction
int vebose = 0 , s , e , b , S , B; // 各种变量
enum Status {HIT , MISS , EVICTION};  // 枚举类型
const char* status_str[3] = {" hit"," miss"," eviction"};
char message[20];  // 表示最后需要打印的日志信息
int T = 0;  // 表示存入的时刻
// 定义缓存信息的结构体
typedef struct listNode {
    int t;  // 表示权重,相当于 LRU中的使用次数的判断依据
    uint64_t flag;  // 表示需要校验的标志位
} *groupNode,listNode;  // groupNode 相当于 listnode* , 也就是一个组中的数据
groupNode* cache;  // 表示缓存,缓存也就是组的数组
// 1. -h 参数行为
void print_help_page() {
    printf("-h: Optional help flag that prints usage info\n");
    printf("-v: Optional verbose flag that displays trace info\n");
    printf("-s <s>: Number of set index bits (S = 2^s is the number of sets)\n");
    printf("-E <E>: Associativity (number of lines per set)\n");
    printf("-b <b>: Number of block bits (B = 2^b is the block size)\n");
    printf("-t <tracefile>: Name of the valgrind trace to replay\n");
}
// 初始化缓存
void init() {
    cache = (groupNode*) malloc (sizeof(groupNode) * S);  // cache 表示组的数组,groupNode 表示一个组的首地址,相当于 8 * S
    // 相当于元素为 listNode 的二维数组
    for(int i = 0 ; i < S ; i ++) {
        cache[i] = (listNode*) malloc (sizeof(listNode) * e);
        // 初始化访问次数
        for(int j = 0 ; j < e ; j ++) cache[i][j].t = 0; // 表示没有访问过
    }
}

void destory() {
    // 表示释放缓存空间
    if(cache != NULL) {
        for(int i = 0 ; i < S ; i ++) {
            free(cache[i]);
        }
        free(cache);  // 释放申请的空间
    }
}
/**
 *  统一的一个设置指定的位置缓存相应数据的函数
 */
void setResult(enum Status type , int group_pos , uint64_t flag , int pos) {
    result[type] ++;
        cache[group_pos][pos].flag = flag;
        cache[group_pos][pos].t = T;  // 表示最新的事件,最不容易被淘汰
        if(vebose) strcat(message , status_str[type]);
}
// 核心逻辑: 查询缓存,其实数据存储和数据访问的逻辑类似,由于不用关心数据的根新和底层的同步
/**
 * 需要提供的参数: 组的序号 group_pos 和 标记号 flag
 *  data load:
 *   如果命中缓存直接设置状态即可
 *   如果没有命中缓存,首先查看有没有空白行,如果没有空白行,那么就需要找到最小的位置进行驱逐
 * 
 *  data store:
 *   由于不管数据和底层的交互逻辑,所以基本的方法还是一样的,所以可以共用一个函数  
 */
void findCache(int group_pos , uint64_t  flag) {
    // 此时需要遍历的组就是 cache[group_pos]
    // 分别记录空行的位置和最应该被淘汰的位置的索引
    int empty_line = -1 , min_pos = 0;
    for(int i = 0 ; i < e ; i ++) {
        if(cache[group_pos][i].t == 0) {
            // 表示空行
            empty_line = i;
            continue;
        }
        if(cache[group_pos][i].flag == flag) {
            // 表示此时命中缓存了,可以直接返回
            setResult(HIT , group_pos , flag , i);
            return ;
        }
        // 记录最小的 t 的位置
        if(cache[group_pos][i].t < cache[group_pos][min_pos].t) {
            min_pos = i;
        }
    }
    //  没有返回表示没有命中缓存,如果有空行可以把数据填入到空行中
    // if(empty_line != -1) {
    //     setResult(MISS , group_pos , flag , empty_line);
    // } else {
    //     setResult(EVICTION , group_pos , flag , min_pos);
    //     // result[MISS] ++;
    //     // strcat(message , status_str[MISS]);
    // }
    if(empty_line == -1) {
        result[MISS] ++;
        strcat(message , status_str[MISS]);
        setResult(EVICTION , group_pos , flag , min_pos);
    } else {
        setResult(MISS , group_pos , flag , empty_line);
    }

}
int main(int argc , char* argv[])
{
    // 1. 参数解析
    // 1.1 首先解析 -h 参数
    if(argc < 2) {
        printf("bad options!\n");
        exit(1);
    } else if(argc == 2) {
        if(strcmp(argv[1] , "-h") == 0) {
            print_help_page();
            exit(0);
        } else {
            printf("bad options!\n");
            exit(1);
        }
    }
    // 1.2 解析其他参数
    char* file_name;
    int oc;
    while((oc = getopt(argc , argv , "vs:E:b:t:")) != -1) {
        switch(oc) {
            case 'v':
                vebose = 1;
                break;
            case 's':
                s = (*optarg) - BASE_NUM;
                S = 1 << s;
                break;
            case 'E': 
                e = (*optarg) - BASE_NUM;
                break;
            case 'b':
                b = (*optarg) - BASE_NUM;
                B = 1 << b;
                break;
            case 't':
                file_name = optarg;
                break;
            default:
                printf("you input a invalild option !");
                exit(1);
        }
    }
    // 2. 可以根据参数开始解析出各种状态的值
    // 这里可以使用 fscanf 函数进行文件内容的读取
    char oper[2];  // 用于记录前面两个字符
    uint64_t addr;  // 记录地址 64bits
    int size;     // 记录大小
    FILE* fp = fopen(file_name , "r");
    init();
    while((fscanf(fp , "%s %lx,%d\n", oper , &addr , &size)) == 3) {
        if(oper[0] == 'I') continue;
        memset(message , 0 , sizeof(message));
        // 获取到地址中的参数,首先取出 s 来
        int group_pos = (addr >> b) & (~(~0u << s));
        // 取出 flag 来
        uint64_t flag = (addr >> (b + s));
        T++; // 时间 + 1
        findCache(group_pos,flag);
        // 注意不算空格
        if(oper[0] == 'M') findCache(group_pos,flag);
        if(vebose == 1) fprintf(stdout , "%c %lx,%d%s\n",oper[0] , addr , size , message);
    }
    printSummary(result[HIT] , result[MISS] , result[EVICTION]);
    destory();
    fclose(fp);
    return 0;
}
