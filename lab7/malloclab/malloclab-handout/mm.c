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
    "a team",
    /* First member's full name */
    "XZW",
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
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
/**
 *   定义一些可能需要使用的宏定义
 */
#define WSIZE 4 // 字
#define DSIZE 8 // 双字
#define CHUNKSIZE (1 << 8)

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

#define GET_PRED(p) ((char*)(p))  // 获取到空闲块的头指针 
#define GET_SUCC(p) ((char*)(p) + WSIZE)  // 获取到空闲块的尾指针
/**
 *  定义一些需要使用的常量
 */
static void* heap_start;  // 指向堆开始的位置,用于记录数组中各个元素的首地址
static void* buckets[10];  // 用于存放第一个指针
/**
 *  定义需要使用的函数
 */
int mm_init(void);
void* mm_malloc(size_t size);
void mm_free(void* ptr);
void* mm_realloc(void* ptr , size_t size);

static void* extendSize(size_t size);
static int selectPosition(size_t size);
static void* mergeBorder(void* bp);
static void headInsert(int index , void* ptr);
static void deleteNode(void* ptr , int index);
static void* firstFit(int index , size_t size);
static void place(void* bp , size_t asize);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // 首先初始化桶的大小
    // 8 - 16  | 16 - 32 | 32 - 64 | 64 - 128 | 128 - 256 | 256 - 512 | 512 - 1024 | 1024 - 2048 | 2048 - 4096(4K) | > 4096
    // 10 * WSIZE + WSIZE +2 * WSIZE + WSIZE
    if((heap_start = mem_sbrk(4 * WSIZE)) == (void*)(-1)) return -1;  // 分配空间失败
    // PUT(heap_start , 0);   // 8 -> 16
    // PUT((heap_start + WSIZE) , 0); // 16 -> 32
    // PUT((heap_start + 2*WSIZE) , 0);  // 32 -> 64
    // PUT((heap_start + 3*WSIZE) , 0);  // 64 -> 128
    // PUT((heap_start + 4*WSIZE) , 0); // 128 -> 256
    // PUT((heap_start + 5*WSIZE) , 0); // 256 -> 512
    // PUT((heap_start + 6*WSIZE) , 0);  // 512 -> 1024
    // PUT((heap_start + 7*WSIZE) , 0); // 1024 -> 2048
    // PUT((heap_start + 8*WSIZE) , 0); // 2048 -> 4096
    // PUT((heap_start + 9*WSIZE) , 0);  // > 4096
    // PUT((heap_start + 10*WSIZE) , 0);  // 填充块
    // PUT((heap_start + 11*WSIZE) , PACK(DSIZE , 1));  // 序言块1
    // PUT((heap_start + 12*WSIZE) , PACK(DSIZE , 1)); // 序言块尾
    // PUT((heap_start + 13*WSIZE) , PACK(0 , 1));  // 空块
    PUT(heap_start , 0);
    PUT((heap_start + WSIZE) , PACK(DSIZE , 1));
    PUT((heap_start + DSIZE) , PACK(DSIZE , 1));
    PUT((heap_start + DSIZE + WSIZE) , PACK(0 , 1));
    for(int i = 0 ; i < 10 ; i ++) buckets[i] = NULL;
    // 进行块的扩展
    if(extendSize(CHUNKSIZE/WSIZE) == NULL) return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    int index = selectPosition(newsize); 
    // 开始寻找空间
    void* bp;
    if((bp = firstFit(index , newsize)) != NULL) {
        place(bp , newsize);
        return bp;
    }
    // 申请空间,每一次只需要申请一个组内的空间
    size_t curSize = MAX(newsize , CHUNKSIZE/WSIZE);  // 16 - 32 24 --> 1 --> 32 (1 << 6)
    if((bp = extendSize(curSize/WSIZE)) == NULL) return NULL;
    place(bp , newsize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    // 释放空间
    size_t csize = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr) , PACK(csize , 0));
    PUT(FTRP(ptr) , PACK(csize , 0));
    // 本来就不再桶中
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
    copySize = GET_SIZE(HDRP(ptr)) - DSIZE;
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

/**
 *  用于分配不同的块,同时将分好的块放入到对应的位置
 */
static void* extendSize(size_t size)
{
    size_t words = (size % 2 == 0) ? size*WSIZE : (size + 1)*WSIZE;
    // 申请空间
    void* bp;
    if((long)(bp = mem_sbrk(words)) == -1) return NULL;  // 分配空间失败
    PUT(HDRP(bp) , PACK(words , 0));
    PUT(FTRP(bp) , PACK(words , 0));
    // 进行边界的合并并且放入到指定的位置,需要合并的块就是没有分配的
    // 设置尾巴
    PUT(HDRP(NEXT_BLKP(bp)) , PACK(0 , 1));
    return mergeBorder(bp);
}

/**
 *  用于判断块的大小应该属于那一个位置
 */
static int selectPosition(size_t size)
{
    // 4096 = 2^12 = 1 >> 12
    // 总共有 10 组,最大索引为 9
    if(size >= 4096) return 9;
    // size >> 4 == 0 --> 0  8 - 16 
    // size >> 5 == 0 --> 1  16 - 32
    int n = 0;
    while((size >> (n + 4)) > 0) n ++;
    return n;
}

/**
 *  用于边界的合并,这一个函数就是就是和 extendSize 一起使用的
 *  针对于已经没有放入到桶中的块
 */
static void* mergeBorder(void* bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t csize = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc) {
        // 需要放入到桶中
        int index = selectPosition(csize);
        headInsert(index , bp);
        return bp;
    }

    else if(prev_alloc && !next_alloc) {
        int nsize = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        int nindex = selectPosition(nsize);
        deleteNode(NEXT_BLKP(bp) , nindex);
        csize += nsize;
        int index = selectPosition(csize);
        // 设置为空闲状态
        PUT(HDRP(bp) , PACK(csize , 0));
        PUT(FTRP(bp) , PACK(csize , 0));
        headInsert(index , bp);
    }

    else if(!prev_alloc && next_alloc) {
        int psize = GET_SIZE(HDRP(PREV_BLKP(bp)));
        int pindex = selectPosition(psize);
        deleteNode(PREV_BLKP(bp) , pindex);
        csize += psize;
        int index = selectPosition(csize);
        PUT(HDRP(PREV_BLKP(bp)) , PACK(csize , 0));
        PUT(FTRP(bp) , PACK(csize , 0));
        bp = PREV_BLKP(bp);
        headInsert(index , bp);
    }

    else if(!prev_alloc && !next_alloc) {
       // 删除前面的节点
       int psize = GET_SIZE(FTRP(PREV_BLKP(bp)));
       int pindex = selectPosition(psize);
       int nsize = GET_SIZE(HDRP(NEXT_BLKP(bp)));
       int nindex = selectPosition(nsize);
       deleteNode(PREV_BLKP(bp) , pindex);
       deleteNode(NEXT_BLKP(bp) , nindex);
       csize += (psize + nsize);
       int index = selectPosition(csize);
       PUT(HDRP(PREV_BLKP(bp)) , PACK(csize , 0));
       PUT(FTRP(NEXT_BLKP(bp)) , PACK(csize , 0));
       bp = PREV_BLKP(bp);
       headInsert(index , bp);
    }
    return bp;
}
/**
 *  把元素插入到指定的位置
 */
static void headInsert(int index , void* ptr)
{
    if(buckets[index] == NULL) {
        // 没有节点
        PUT(GET_PRED(ptr) , 0);
        PUT(GET_SUCC(ptr) , 0);
        buckets[index] = GET_PRED(ptr);
    } else {
        // 头插法
        PUT(GET_PRED(ptr) , 0);
        PUT(GET_SUCC(ptr) , buckets[index]);
        PUT(buckets[index] , GET_SUCC(ptr));
        buckets[index] = GET_PRED(ptr);
    }
}
/**
 *  在指定的链表中删除节点
 */
static void deleteNode(void* ptr , int index)
{
    void* pred = GET(GET_PRED(ptr));
    void* succ = GET(GET_SUCC(ptr));
    if(pred == NULL && succ == NULL) {
        // 只有一个节点
        buckets[index] = NULL;
    } else if(pred == NULL && succ != NULL) {
        // 头节点
        PUT(succ , 0);
        buckets[index] = succ;
    } else if(pred != NULL && succ == NULL) {
        PUT(pred , 0);
    } else if(pred != NULL && succ != NULL) {
        PUT(pred , succ);
        PUT(succ , pred);
    }
}

static void* firstFit(int index , size_t asize)
{
    for(int i = index ; i <= 9 ; i ++) {
        void* bp = buckets[i];
        for( ; bp != NULL ; bp = GET(GET_SUCC(bp))) {
            if(asize <= GET_SIZE(HDRP(bp))) {
                return bp;
            }
        }
    }
    return NULL;
}

static void place(void* bp , size_t asize)
{
    int csize = GET_SIZE(HDRP(bp));
    int index = selectPosition(csize);
    deleteNode(bp , index);
    if((csize - asize) >= 2*DSIZE) {
        // 可以进行分割操作
        PUT(HDRP(bp) , PACK(asize , 1));
        PUT(FTRP(bp) , PACK(asize , 1));
        // 注意此时采用值赋值的方式,可以修改 bp 本身
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp) , PACK((csize - asize) , 0));
        PUT(FTRP(bp) , PACK((csize - asize) , 0));
        mergeBorder(bp); // 直接进行融合
        return ;
    }
    PUT(HDRP(bp) , PACK(csize , 1));
    PUT(FTRP(bp) , PACK(csize , 1));
}