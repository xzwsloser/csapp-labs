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
    "xzw@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};
/**
 *  这里采用隐式空闲链表的形式来模拟分配得到的堆内存
 *  mm_malloc采用首次适配的方式
 *  不用定义结构体只用操作内存即可
 *  可以使用的函数:
 *  void *mem sbrk(int incr)  扩展堆内存
 *  void *mem heap lo(void)   指向堆内存开始的第一个字节
 *  void *mem heap hi(void)   堆内存的最后一个字节
 *  size t mem heapsize(void) 堆的大小
 *  size t mem pagesize(void) 页的大小(Linux下为4K)
 */
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t))) // ALIGN(4) --> 4 + 7 = 11 0000 1011 --> 8

#define WSIZE 4  // 表示单字

#define DSIZE 8  // 表示双字

#define CHUNKSIZE (1 << 12)  // 4K页面大小,扩容的时候使用

#define PACK(size,alloc) ((size) | (alloc))  // 注意宏定义的写法

#define PUT(p , val) (*(unsigned int*)p = (val)) // 表示指针的放置方式

#define GET(p) (*(unsigned int*)p)  // 获取到 p 位置的值

#define GET_SIZE(p) (GET(p) & ~0x7)  // 获取到大小

#define GET_ALLOC(p) (GET(p) & 0x1) // 获取到是否分配

#define HDRP(bp) ((char*)(bp) - WSIZE)  // bp 指向有效载荷,获取到头部的地址

#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)  // 大小包含头部和尾部

#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(((char*)(bp) - WSIZE)))  // 获取到后面一个块

#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE)))  // 获取到前面一个块
/* 
 * mm_init - initialize the malloc package.
    作用: 初始化一块堆内存,需要利用到 void *mem sbrk(int incr) 函数
    这一个函数用于分配初始化的堆内存,最大的大小为 20MB

    1. 初始结构:  4B 填充块 + 4B 头部 + 4B 尾 + 4B 结尾
    2. 进行扩充操作即可
 */
int mm_init(void)
{
    // 1. 首先进行扩容操作 4 * ESIZE,当成字节对齐即可
    void* heap_start;  // 表示堆的开始地址
    // 分配内存失败,保留 errorno
    if((heap_start = mem_sbrk(4 * WSIZE)) == (void*)-1) return -1;
    // 2. 初始化上面的各种结构,直接当成二进制即可
    PUT(heap_start , PACK(WSIZE , 0));
    // header
    PUT((heap_start + WSIZE) , PACK(WSIZE , 1));
    // footer
    PUT((heap_start + 2 * WSIZE) , PACK(WSIZE , 1));
    // end
    PUT((heap_start + 3 * WSIZE) , PACK(0 , 1));
    // 3. 最后分配剩余的空间即可,也可以直接进行堆的拓展
    if(mem_sbrk(CHUNKSIZE) == (void*)(-1)) return -1;
    return 0;
}

/**
 *  进行空闲空间的合并
 */
static void* coalesce(void* bp) 
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    // case1  前面和后面的块都分配了
    if(prev_alloc && next_alloc) {
        return bp;
    }
    // case2  前面的分配,后面的没有分配
    else if(prev_alloc && !next_alloc) {
        size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        // 设置
        PUT(HDRP(bp) , PACK(next_size + size , 0));
        PUT(FTRP(NEXT_BLKP(bp)) , PACK(next_size + size , 0));
    }

    // case3 前面的空闲,后面的分配
    else if(!prev_alloc && next_alloc) {
        size_t prev_size = GET_SIZE(FTRP(PREV_BLKP(bp)));
        // 设置
        PUT(HDRP(PREV_BLKP(bp)) , PACK(prev_size + size , 0));
        PUT(FTRP(bp) , PACK(prev_size + size , 0));
        bp = PREV_BLKP(bp);
    }
    // case4 前面和后面的都空闲
    else if(!prev_alloc && !next_alloc) {
        size_t prev_size = GET_SIZE(FTRP(PREV_BLKP(bp)));
        size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        // 设置
        PUT(HDRP(PREV_BLKP(bp)) , PACK(prev_size + next_size + size , 0));
        PUT(FTRP(NEXT_BLKP(bp)) , PACK(prev_size + next_size + size , 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

static void* extend_heap(size_t words) 
{
    char* bp;  // 记录开始的位置
    size_t size; // 大小
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    // 分配空间
    if((long)(bp = mem_sbrk(size)) == -1) return NULL;
    // 把初始的空间设置为没有分配的即可
    PUT(HDRP(bp) , PACK(size , 0));
    PUT(FTRP(bp) , PACK(size , 0));
    PUT(HDRP(NEXT_BLKP(bp)) , PACK(0 , 1)); // 结束位置
    // 空间刚好,并且前面和后面都是已经分配了的块
    return coalesce(bp);
}


static void* find_fit(size_t asize) 
{
    // 找到合适的可以放置 asize 的位置 
    if(asize > mem_heapsize()) return NULL;  // 没有适合的空间
    // 首先找到开始节点的位置
    void* heap_start = mem_heap_lo();
    // 还是首先找到第一个块的为子
    void* cur = heap_start + 4 * WSIZE;
    while(GET_ALLOC(HDRP(cur)) && !GET_SIZE(HDRP(cur))) {
        // 判断
        size_t cur_size = GET_SIZE(HDRP(cur));
        size_t cur_alloc = GET_ALLOC(HDRP(cur));
        if(!cur_alloc && cur_size) {
            // 找到了
            return cur;
        }
        cur = NEXT_BLKP(cur);
    } 
    return NULL;
}
/**
 *  此时已经处理好了内存对齐了,所以可以直接进行放置和分割操作 
 */
static void place(void* bp , size_t asize)
{
    size_t size = GET_SIZE(HDRP(bp));
    // 注意这里只是操作已经有的块
    if((size - asize) > 2 * DSIZE) {
        // 可以进行放置了
        PUT(HDRP(bp) , PACK(asize , 1));
        PUT(FTRP(bp) , PACK(asize , 1));  // 注意此时 size 已经发生了改变了
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp) , PACK(size - asize , 0));
        PUT(HDRP(bp) , PACK(size - asize , 0)); // 至少需要一个空间
    } 

    else {
        PUT(HDRP(bp) , PACK(size , 1));
        PUT(HDRP(bp) , PACK(size , 1));  // 此时发生内存对齐
    }
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE); // ALIGN(3 + 8) 11 + 7 19 --> 0001 0011 --> 16
    // 也就是 3 + 8 = 11 --> 16
    // 开始进行空间的分配了
    // 采用首次匹配法来找到合适的空间
    void* bp;
    if((bp = find_fit(newsize)) != NULL) {
        place(bp , newsize); // 其实指针也是值传递
        return bp;
    }

    int extendsize = newsize > CHUNKSIZE ? newsize : CHUNKSIZE;
    if((bp = extend_heap(extendsize)) == NULL) return NULL;
    place(bp , newsize);
    return bp;
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    // 直接把这一块空间释放并且合并周围的空间
    size_t csize = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr) , PACK(csize , 0));
    PUT(FTRP(ptr) , PACK(csize , 0));
    coalesce(ptr);  // 合并周围的空间
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void* oldptr = ptr;  // 记录原来的指针位置,方便释放原来的空间
    if(size == 0) {
        mm_free(ptr);
        return NULL;
    } 
    // 分配空间并且完成数据的拷贝
    size_t copySize = GET_SIZE(HDRP(oldptr)) - 2 * WSIZE;
    void* newPtr = mm_malloc(size);
    // 注意这里的 size 就是时机的长度
    if(size <= copySize) {
        memcpy(newPtr , oldptr , size);
    } else if(size > copySize) {
        memcpy(newPtr , oldptr , copySize);
    }
    mm_free(oldptr);
    return newPtr;
}














