#include "cachelab.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#define BASE_NUM 48
#define LINE_SIZE 100
#define ADDR_SIZE 32
#define MASK 0xFFFFFFFF
typedef struct input_message {
    // 表示选项
    char opt;
    // 表示高速缓存区中的地址
    __uint64_t addr; 
}input_message;
// 1. -h 参数行为
void print_help_page() {
    printf("-h: Optional help flag that prints usage info\n");
    printf("-v: Optional verbose flag that displays trace info\n");
    printf("-s <s>: Number of set index bits (S = 2^s is the number of sets)\n");
    printf("-E <E>: Associativity (number of lines per set)\n");
    printf("-b <b>: Number of block bits (B = 2^b is the block size)\n");
    printf("-t <tracefile>: Name of the valgrind trace to replay\n");
}
int pow2(int num) {
    // 求解 2 ^ num
    int res = 1;
    for(int i = 0 ; i < num ; i ++) {
        res *= 2;
    }
    return res;
}
int* malloc_cache(int s , int e , int b) {
    // 1. 首先可以获取组数量,行数量和块的大小
    int S = pow2(s);
    int B = pow2(b);
    // 2. 开始求解需要的总的空间大小
    // 这里不妨假设 m = 32
    // t = m - s - b
    // 那么此时一组中需要的大小就是 1位(有效位置) + m - s - b 位 + 2^b 个字节
    // 都可以分配 int,转换为十进制进行比较
    // 这里不用考虑大小,假设已经吗满足了内存对齐的要求,为了提高精度,使用int
    // 需要的字节数量为 S * e * (1 + 1 + 2 ^ b ) * 4
    int size = S * e * (1 + 1 + B) * sizeof(int);
    int* start_addr = (int*)malloc(size);
    memset(start_addr , 0 , size);
    return start_addr;
}

input_message read_line(FILE* fp) {
    // 1. 读取一行
    char buf[LINE_SIZE];
    char ch;
    int i = 0;
    ch = fgetc(fp);
    if(ch == EOF) {
        input_message res = {EOF , 0};
        return res;
    }
    while(ch != EOF && ch != '\n') {
        buf[i] = ch;
        i ++;
        ch = fgetc(fp);
    }
    // 2. 提取出有用的信息
    input_message res;
    if(buf[0] != ' ') {
        res.opt = 'I';
        res.addr = '0';
    } else {
        res.opt = buf[1];
        // 需要把第二个参数从字符串转换为数字
        int n = strlen(buf);
        int index = 3;
        while(index < n && buf[index] != ',') index ++;
        // 此时需要把字符串 3 - index 的位置转换为数字
        // index - 3 + 1
        char target[index - 2];
        for(int i = 3 ; i <= index ; i ++) {
            target[i - 3] = buf[i];
        }
        // 把target 转换为数字类型
        int this_addr = atoi(target);
        // 封装对象返回
        res.addr = (__uint64_t)this_addr;
    }  
    return res;
}
/**
 *  data load:
 *  从缓存中读取数据:
 *  1. 如果数据存在于缓存中,直接命中并且返回即可
 *  2. 如果数据不存在于缓存中,首先更新缓存并且返回即可,会造成驱逐的效果
 */
void data_load_handler(int S , int e , int B , int* start_addr , int* hit_count , int* miss_count , int* eviction_count, int addr_s , int addr_t , int addr_b ) {
    // 作用: 统计命中数量和没有命中的数量
    // start_addr 的结构 , S 组 , e 行 , 1 + 1 + B 个单位
    int* ptr = start_addr;
    // 1. 首先找到组 0 - S-1
    // 没有命中,强制不命中
    if(addr_s >= S || addr_s < 0) {
        (*miss_count) += 1;
        return ;
    }
    // 或者偏移量过大,导致工作范围大于缓存范围,那么就会导致无法缓存
    if(addr_b >= B) {
        (*miss_count) += 1;
        return ;
    }
    // 1.1 定位到对应的组别
    ptr = ptr + (2 + B) * e * addr_s;
    // 1.2 找到组别中标志为为 t 的位置
    int* cur = ptr;
    int* end = ptr + (2 + B) * e;
    while((cur != end) && (*(cur + 1) != addr_t)) {
        cur = cur + (2 + B);
    }
    if(cur == end) {
        (*miss_count) += 1;
        // 开始更新缓存,假设此时采取的策略就是更新第一个位置(之后可以采用LRU的方法)
        *ptr = 1;
        *(ptr + 1) = addr_t;
        (*eviction_count) += 1;
        *(miss_count) += 1;
        // 后面的默认可以命中缓存即可,注意逻辑
        return ;
    } else {
        // 表示找到对应的位置,终于可以命中了
        // 还是首先判断有效位置
        if((*cur) == 0) {
            (*miss_count) += 1;
            (*cur) = 1;
            *(cur + 1) = addr_t;
            // 相当于后面的都可以命中了
            return ;
        }
        (*hit_count) += 1;
        return ;
    }
}
/**
 *  data store
 *  向缓存中写入数据,也就是原来没有数据,所以不用考虑是否命中的情况
 *  如果找不到对应位置直接进行驱逐即可
 */
void data_store_handler(int S , int e , int B , int* start_addr , int* hit_count , int* missing_count , int* eviction_count , int addr_s ,int addr_t ,int addr_b) {
    if(addr_s >= S || addr_s < 0 || addr_b >= B) {
        *(missing_count) += 1;
        return ;  // 无法存储数据到缓存中
    }
    int* ptr = start_addr;
    // 只需要查看是否需要驱逐即可
    ptr = ptr + (2 + B) * e * addr_s;
    int* cur = ptr;
    int* end = ptr + (2 + B) * e;
    while(cur != end && (*(cur + 1) != addr_t)) {
        cur = cur + (2 + B);
    }
    if(cur == end) {
        // 没有空间进行驱逐
        *(eviction_count) += 1;
        *ptr = 1;
        *(ptr + 1) = addr_t;
        return ;  // 此时数据的存储完成
    } else {
        // 此时可以进行数据的存储了,不用进行驱逐了
        *(hit_count) += 1;
        *ptr = 1;
        *(ptr + 1) = addr_t;
        return ;
    }
}
// 核心函数用于判断指令是否命中,或者指令需要放逐
void instr_handler(int s , int e , int b , int* start_addr , input_message ipt , int* hit_count , int* miss_count , int* eviction_count , int* size) {
    // 分析 32 位的地址结构:
    // t 为标记位 s 位索引号 b 位块偏移量
    // 1. 首先取得这些字段的十进制编码
    // int t = ADDR_SIZE - s - b;
    int addr = ipt.addr;
    // 注意 t 在最高位
    unsigned mask = MASK;
    // t 的十进制表示
    int addr_t = (mask >> (b + s)) & (addr >> (b + s));
    // s 的十进制表示
    int addr_s = (mask >> (ADDR_SIZE - s)) & (addr >> b);
    // b 的十进制表示
    int addr_b = (mask >> (ADDR_SIZE - b)) & addr;
    // 2. 接下来分别处理 L , M , S 的情况
    switch(ipt.opt) {
        case 'L':
            data_load_handler(pow2(s), e , pow2(b),start_addr,hit_count,miss_count,eviction_count,addr_s , addr_t,addr_b);
        break;
        
        case 'S':
            data_store_handler(pow2(s) , e , pow2(b) , start_addr , hit_count , miss_count , eviction_count , addr_s , addr_t , addr_b);
        break;

        case 'M':
            data_load_handler(pow2(s), e , pow2(b),start_addr,hit_count,miss_count,eviction_count,addr_s,addr_t,addr_b);
            data_store_handler(pow2(s) , e , pow2(b) , start_addr , hit_count , miss_count , eviction_count , addr_s , addr_t , addr_b);
        break;
    }

}
void trace_handler(int s , int e , int b , char* file_name , int* hit_count , int* miss_count , int* eviction_count) {
    // 用于读取信息并且不断进行统计
    // 1. 进行空间的分配
    // 1.1. 之后可以利用指针的运算进行取址计算
    int* start_addr = malloc_cache(s , e , b);
    int size = pow2(s) * e * pow2(b);  // 剩余空间的数量,一行中可以存储 B 个 int(假设字节对齐)
    // 2. 开始循环读取需要的文件并且获取到其中的各种信息
    // 2.1 首先还是打开文件
    FILE* fp = fopen(file_name , "r");
    input_message ipt;
    ipt = read_line(fp);
    while(ipt.opt != EOF) {
        // printf("%c %d\n",ipt.opt , ipt.addr);
        if(ipt.opt == 'I') {
            ipt = read_line(fp);
            continue;
        }
        // 开始处理结果
        instr_handler(s , e , b , start_addr , ipt , hit_count , miss_count , eviction_count, &size);
        ipt = read_line(fp);
    }
    free(start_addr);
    fclose(fp);
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
    int s , e , b ; 
    char* file_name;
    bool pr_log = false;
    int oc;
    while((oc = getopt(argc , argv , "vs:E:b:t:")) != -1) {
        switch(oc) {
            case 'v':
                pr_log = true;
                printf("%d \n",pr_log);
                break;
            case 's':
                s = (*optarg) - BASE_NUM;
                break;
            case 'E': 
                e = (*optarg) - BASE_NUM;
                break;
            case 'b':
                b = (*optarg) - BASE_NUM;
                break;
            case 't':
                file_name = optarg;
                break;
            default:
                printf("you input a invalild option !");
                exit(1);
        }
    }
    // 2. 开始读取并且解析文件
    int hit_count , miss_count , eviction_count;
    trace_handler(s, e , b , file_name , &hit_count , &miss_count , &eviction_count);
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
