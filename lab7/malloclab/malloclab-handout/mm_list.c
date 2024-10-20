/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "XZW",
    /* First member's email address */
    "xzw@eic.hust.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};
/**
 *   采用显示链表的形式对于数据进行存储操作
 *   这样规定,如果 pred | succ = 0 ---> 表示指向了 NULL 
 */
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/**
 *  这里存放着一些需要使用的宏定义
 */
/***
 *  以下是一些需要使用到的宏定义
 */
#define WSIZE 4 // 字
#define DSIZE 8 // 双字
#define CHUNKSIZE (1 << 6)

#define MAX(x , y) ((x) > (y) ? (x) : (y))  // 最大值

#define PACK(size , alloc) ((size) | (alloc)) // 头部和脚部的值

#define GET(p) (*(unsigned int*)(p)) // 获取到 p 处的值
#define PUT(p , val) (*(unsigned int*)(p) = (val)) // 把 val 放置到 p

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char*)(bp) - WSIZE)  // 获取到首部的地址,注意这里 char* 可以被翻译为数字
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) // 获取到尾部的值

#define NEXT_BLKP(p) ((char*)(p) + GET_SIZE(((char*)(p) - WSIZE)))  // 获取到下一个块的地址
#define PREV_BLKP(p) ((char*)(p) - GET_SIZE(((char*)(p) - DSIZE)))  // 获取到上一个块的地址

/**
 * 这里存放着与链表相关一些宏定义(指针操作)
 * 本质上空间位置还是相邻的
 */
#define GET_PRED(bp) ((char*)(bp))  // 获取到 pred 位置的地址
#define GET_SUCC(bp) ((char*)(bp) + WSIZE) // 获取到 succ 位置的地址
// #define GET_NEXT(pred) (((char*)(pred) + WSIZE)) // 根据  pred 获取到 succ
/**
 * 这里定义一些需要使用的常量
 */
static void* heap_start;  // 表示堆开始的位置
static void* list_pred;  // 表示链表的第一个节点的 pred 指针
static void* list_succ;    // 表示链表最后一个节点的 succ 指针

/**
 *  这里定义一些需要使用的函数
 */

int mm_init(void);
void* mm_malloc(size_t size);
void mm_free(void* ptr);
void* mm_realloc(void* ptr , size_t size);


static void* extendSize(size_t asize);
static void* mergeBorder(void* bp);
static void* firstFit(size_t asize);
static void place(void* bp , size_t asize);
static void delete_node(void* bp);  // 取消 bp 和前面节点和后面节点之间的联系
static void head_insert(void* bp);
/* 
 * mm_init - initialize the malloc package.
 * void* 类型相当于泛型指针
 */
int mm_init(void)
{
    // 1. 初始化特定结构
    if((heap_start = mem_sbrk(4 * WSIZE)) == (void*)(-1)) {
        printf("[mm_init] failed to allocate a free space !\n");
        return -1;
    }
    PUT(heap_start , 0);
    PUT((heap_start + WSIZE) , PACK(DSIZE , 1));
    PUT((heap_start + DSIZE) , PACK(DSIZE , 1));
    PUT((heap_start + DSIZE + WSIZE) , PACK(0 , 1));
    heap_start += DSIZE;  
    list_pred = NULL;   
    list_succ = NULL;  // 表示指向 NULL
    // 2. 进行扩容操作
    if(extendSize(CHUNKSIZE/WSIZE) == NULL) return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if(size == 0) return NULL;
    int newsize = ALIGN(size + SIZE_T_SIZE);
    // 首先还是需要找到一个合适的位置,这里考虑使用首次匹配的方式
    void* bp = firstFit(newsize);
    if(bp != NULL) {
        // 找到了匹配位置
        place(bp , newsize);
        return bp;
    }
    // 需要重新分配空间
    int curSize = MAX(newsize , CHUNKSIZE);
    // 分配空间
    if((bp = extendSize(curSize/WSIZE)) == NULL) return NULL;
    place(bp , newsize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    // 释放空间并且把释放的空间插入到链表头
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr) , PACK(size , 0));
    PUT(FTRP(ptr) , PACK(size , 0));
    // 这里可以使用头插入法进行元素的插入
    head_insert(ptr);
    mergeBorder(ptr); // 合并空间
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = GET_SIZE(HDRP(ptr)) - DSIZE;
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
/**
 *  void* 类型的指针,如果直接赋值,那么就可以当成整数使用了,如果赋予一个地址指针来使用了
 */
static void* extendSize(size_t asize) 
{
    void* bp;  // 用于记录请求的成功和失败
    size_t newsize;  // 用于记录请求大小
    newsize = (asize % 2 == 0) ? (asize * WSIZE) : ((asize + 1) * WSIZE);
    // 申请空间
    if((long)(bp = mem_sbrk(newsize)) == -1) return NULL;  // 分配失败
    // 分配成功之后需要初始化,此时bp就是头指针
    PUT(HDRP(bp) , PACK(newsize , 0));
    PUT(FTRP(bp) , PACK(newsize , 0));
    PUT(HDRP(NEXT_BLKP(bp)) , PACK(0 , 1));
    // 看一下运算是否正确
    // 分配指针空间,这就是新的一块空间,需要插入到头部的位置
    if(list_pred == NULL && list_succ == NULL) {
        // 此时链表中没有元素
        PUT((GET_PRED(bp)) , 0);
        PUT((GET_SUCC(bp)) , 0); 
        list_pred = GET_PRED(bp); // 指向新的头节点
        list_succ = GET_SUCC(bp); // 指向新的尾节点

    } else {
        // 链表中存在数据,此时利用头插进行插入
        PUT((GET_PRED(bp)) , 0);
        PUT((GET_SUCC(bp)) , list_pred); 
        // 此时需要放置地址到 list_pred 中
        PUT(list_pred , GET_SUCC(bp));  // 注意此时后面的是指针,前面的是地址
        list_pred = GET_PRED(bp);
    }
    // 注意最后放置尾巴节点
    return mergeBorder(bp); // 表示开始合并边界
}

static void* mergeBorder(void* bp)
{
    // 还是首先获取到前后的空闲状况
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if(prev_alloc && next_alloc) {
        return bp;
    }
    else if(prev_alloc && !next_alloc) {
        // 此时需要合并后面的块
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        // 注意此时不用移动指针了
        delete_node(NEXT_BLKP(bp));
        PUT(HDRP(bp) , PACK(size , 0));
        PUT(FTRP(bp) , PACK(size , 0));  // 注意此时不需要运算即可
        //  同时需要删除后面的指针 
        // 头插法
        delete_node(bp);
        head_insert(bp);
    }
    else if(!prev_alloc && next_alloc) {
        // 此时需要调整前面指针的指向
        size += GET_SIZE(FTRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)) , PACK(size , 0));
        PUT(FTRP(bp) , PACK(size , 0));
        // 调整指针指向
        // 记录地址
        delete_node(bp);  // 消除关联
        bp = PREV_BLKP(bp);
        delete_node(bp);
        head_insert(bp);
    }

    else if(!prev_alloc && !next_alloc) {
        size += GET_SIZE(FTRP(PREV_BLKP(bp)));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        // 设置标志为
        PUT(HDRP(PREV_BLKP(bp)) , PACK(size , 0));
        PUT(FTRP(NEXT_BLKP(bp)) , PACK(size , 0));
        // 调整指针指向
        // 消除自己的指向
        delete_node(bp);
        // 擅长后面的指针的指向
        delete_node(NEXT_BLKP(bp));
        bp = PREV_BLKP(bp);
        delete_node(bp);
        head_insert(bp);
    }
    return bp;
}

static void* firstFit(size_t asize)
{
    // 作用: 找到和是的位置
    void* bp = list_pred;
    for( ; bp != NULL ; bp = GET(GET_SUCC(bp))) {
        if(asize <= GET_SIZE(HDRP(bp))) {
            return bp; // 直接返回
        }
    }
    return NULL;
}

static void place(void* bp , size_t asize)
{
    // 作用: 进行元素的分割和放置
    size_t csize = GET_SIZE(HDRP(bp));
    // 这里多出来的位置就是一个 头尾,加上一个pred和succ
    if((csize - asize) >= (2 * DSIZE)) {
        // 正常设置
        PUT(HDRP(bp) , PACK(asize , 1));
        PUT(FTRP(bp) , PACK(asize , 1));
        // 利用头插法进行插入
        delete_node(bp); // 消除关联
        bp = NEXT_BLKP(bp);
        head_insert(bp);
        // 分配大小
        PUT(HDRP(bp) , PACK((csize - asize) , 0));
        PUT(FTRP(bp) , PACK((csize - asize) , 0));
        return ;
    }
    // 直接分配
    PUT(HDRP(bp) , PACK(csize , 1));
    PUT(FTRP(bp) , PACK(csize , 1));
    // 需要删除这一个块,也就是建立前后节点的关系
    delete_node(bp);
    return ;
}
/**
 *  用于断开前面和后面节点的联系
 */
static void delete_node(void* bp) 
{
    void* temp_pred = GET_PRED(bp);
    void* temp_succ = GET_SUCC(bp);
    void* pred = GET(temp_pred);
    void* succ = GET(temp_succ);
    if(pred == NULL && succ == NULL) {
        // 此时只有一个节点
        list_pred = NULL;
        list_succ = NULL;  // 表示取消节点
    } else if(pred == NULL && succ != NULL) {
        // 头节点
        PUT(succ , 0);
        list_pred = succ;
    } else if(pred != NULL && succ == NULL) {
        // 尾节点
        PUT(pred , 0);
        list_succ = pred;
    } else if(pred != NULL && succ != NULL) {
        // 中间节点
        PUT(pred , succ);
        PUT(succ , pred);
    }
}

static void head_insert(void* bp)
{
    if(list_pred == NULL) {
        PUT(GET_PRED(bp) , 0);
        PUT(GET_SUCC(bp) , 0);
        list_pred = GET_PRED(bp);
        list_succ = GET_SUCC(bp);
    } else {
        PUT(GET_SUCC(bp) , list_pred);
        PUT(list_pred , GET_SUCC(bp));
        PUT(GET_PRED(bp) , 0);
        list_pred = GET_PRED(bp);
    }
}