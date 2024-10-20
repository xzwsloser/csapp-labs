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
    "xzw",
    /* First member's email address */
    "xzw@eic.hust.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
/**
 *   size = 1 --> (1 + 7) & 最后三位为 0 ---> 8
 *   size = 2 --> 9 --> 8
 *   ...
 *   size = 9 ---> 16 --> 16
 *   size = 10 ---> 17
 */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7) // 对于一个 size 进行字节对齐


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))  // = 8
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
 *  这里定义以下需要使用到的函数和需要实现的函数
 */
int mm_init(void);  // 初始化自己的堆
void* mm_malloc(size_t size); // 分配空间
void mm_free(void* bp);  // 释放分配的空间
void* mm_realloc(void* bp , size_t size); // 重新分配空间

static void* extendSize(size_t size); // 扩容
static void* mergeBorder(void* bp);  // 用于合并区间
static void* findFit(size_t asize);  // 用于找到可以存放 asize 的块,并且返回
static void* bestFit(size_t asize);
static void place(void* bp , size_t t); // 用于放置数据在 bp 的位置并且进行区间的分割

/**
 *  这里是需要的一些全局变量
 */
static void* heap_start;  // 指向第一个块开始的位置

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // 1. 分配 4个字的空间用于存放默认数据
    if((heap_start = mem_sbrk(4 * WSIZE)) == (void*)(-1)) {
        printf("[mm_init],failed to allocate room !\n");
        return -1;
    }
    PUT(heap_start , 0);  // 填充块
    PUT((heap_start + WSIZE) , PACK(DSIZE , 1));
    PUT((heap_start + DSIZE) , PACK(DSIZE , 1));
    PUT((heap_start + WSIZE + DSIZE) , PACK(0 , 1));
    heap_start += DSIZE;
    // 分配空间
    if(extendSize(CHUNKSIZE/WSIZE) == NULL) return -1; // 分配失败 
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if(size == 0) return NULL;
    int newsize = ALIGN(size + SIZE_T_SIZE);  // 表示此时需要对齐到 size 的位置
    // 得到的 newsize 就是需要的空间
    void* bp = findFit(newsize);
    if(bp != NULL) {
        place(bp , newsize);  // 初始化放置数据的这一个块,考虑分割
        return bp;
    }
    // 空间不足需要重新分配
    int curSize = MAX(newsize , CHUNKSIZE);
    if((bp = extendSize(curSize/WSIZE)) == NULL) return NULL;
    // 此时 bp 的位置可以放置下 bp
    place(bp , newsize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr) , PACK(size , 0));
    PUT(FTRP(ptr) , PACK(size , 0));
    mergeBorder(ptr);
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
    // 这里可以获取到实际的长度,比如 malloc(5) 一定在 malloc(3) 中被截断
    // 说明这里的长度有问题,但是如何获取到原来 malloc的长度 ?
    // 所以我只用赋值 size 即可
    // 如果 size > copySize 全部赋值即可
    copySize = GET_SIZE(HDRP(oldptr)) - DSIZE;  // 得到的是字节对齐之后的长度
    // copySize > size ==> 赋值 size
    // copySize < size ==> 赋值全部
    if(copySize > size) copySize = size;
    memcpy(newptr , oldptr , copySize);
    mm_free(oldptr);
    return newptr;
}
/**
 *   用于扩展自定义的堆的容量
 */
static void* extendSize(size_t size)
{
    // 1. 首先确定实际需要的空间,也就是需要考虑内存对齐
    int newSize = (size % 2 == 0) ? size * WSIZE : (size + 1) * WSIZE;
    // 2. 分配空间
    void* bp;
    if((bp = mem_sbrk(newSize)) == (void*)(-1)) return NULL;
    // 3. 分配成功,此时 bp 指向结尾的下面,直接牺牲调结尾的部分
    PUT(HDRP(bp) , PACK(newSize , 0));
    PUT(FTRP(bp) , PACK(newSize , 0));
    // 4. 放置结尾的位置
    // 注意结尾的表示方式
    PUT(HDRP(NEXT_BLKP(bp)) , PACK(0 , 1));
    // 5. 进行合并操作
    return mergeBorder(bp);
}
/**
 *  用于合并边界
 */
static void* mergeBorder(void* bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t curSize = GET_SIZE(HDRP(bp));
    /* case1  */
    if(prev_alloc && next_alloc) {
        return bp;
    }

    else if(prev_alloc && !next_alloc) {
        curSize += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp) , PACK(curSize , 0)); // 注意此时 FTRP 已经发生了改变
        PUT(FTRP(bp) , PACK(curSize , 0));
    }

    else if(!prev_alloc && next_alloc) {
        curSize += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp) , PACK(curSize , 0));
        PUT(HDRP(PREV_BLKP(bp)) , PACK(curSize , 0));
        bp = PREV_BLKP(bp);
    }

    else if(!prev_alloc && !next_alloc) {
        curSize += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)) , PACK(curSize , 0));
        PUT(FTRP(NEXT_BLKP(bp)) , PACK(curSize , 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

static void* findFit(size_t asize) 
{
    void* bp = heap_start;
    // 注意终止条件
    for( ; GET_SIZE(HDRP(bp)) > 0 ; bp = NEXT_BLKP(bp)) {
        if(!GET_ALLOC(HDRP(bp)) && (GET_SIZE(HDRP(bp))) >= asize) {
            return bp;  // 此时返回的就是一个地址
        }
    }
    return NULL;
}
/**
 *  目标,把 大小为 asize 的块放入到 bp 的位置
 *  asize 就是实际块的大小(包含了头部和尾部)
 */
static void place(void* bp , size_t asize)
{
    // 是否可以分割,并且至少需要分割出来 DSIZE 的空间,所以最小的差值为 2*DSIZE
    size_t csize = GET_SIZE(HDRP(bp));
    if((csize - asize) >= (2 * DSIZE)) {
        // 可以进行分割
        PUT(HDRP(bp) , PACK(asize , 1));
        PUT(FTRP(bp) , PACK(asize , 1));
        // 这里只用关注地址的运算即可
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp) , PACK((csize - asize) , 0));
        PUT(FTRP(bp) , PACK((csize - asize) , 0));
        mergeBorder(bp);
        // 进行合并操作
        return ;
    }
    // 否则不可以进行分割
    PUT(HDRP(bp) , PACK(csize , 1));
    PUT(FTRP(bp) , PACK(csize , 1));
}
/**
 *  采用最佳匹配的方式将进行匹配,也就是找到最小的满足要求的块
 */
static void* bestFit(size_t asize) 
{
    void* bp = heap_start;
    void* best_bp;  // 记录最佳匹配的位置
    int min_size = __INT_MAX__; // 表示块的最大长度
    for( ; GET_SIZE(HDRP(bp)) > 0 ; bp = NEXT_BLKP(bp)) {
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            if(min_size > GET_SIZE(HDRP(bp))) {
                min_size = GET_SIZE(HDRP(bp));
                best_bp = bp;
            }
        }
    }
    if(min_size == __INT_MAX__) return NULL;
    return best_bp;
}