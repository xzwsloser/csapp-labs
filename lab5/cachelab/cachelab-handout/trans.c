/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char trans_desc_handle32[] = "Transpose submission";
void handler_32(int M, int N, int A[N][M], int B[M][N])
{

    int temp1 , temp2 , temp3 , temp4 , temp5 , temp6 , temp7 , temp8;
    // int temp1 , temp2 , temp3 , temp4;

    // 注意分析地址的变化,这里冲突的原因就是地址的间隔使得 8 * 8 中,B 的地址会被 A 覆盖调
    // 采用矩阵分块的方式进行优化
    for(int i = 0 ; i < N ; i += 8) {
        for(int j = 0 ; j < M ; j += 8) {
            // 接下来只用关注一个块的逻辑
            // 此时 A 中所有位置都存储缓存了,之后都可以命中了
            for(int k = j ; k < j + 8 ; k ++) {
                temp1 = A[i][k];
                temp2 = A[i + 1][k];
                temp3 = A[i + 2][k];
                temp4 = A[i + 3][k];
                temp5 = A[i + 4][k];
                temp6 = A[i + 5][k];
                temp7 = A[i + 6][k];
                temp8 = A[i + 7][k];
             
                // 复制给 B 中对应的位置
                B[k][i] = temp1;
                B[k][i + 1] = temp2;
                B[k][i + 2] = temp3;
                B[k][i + 3] = temp4;
                B[k][i + 4] = temp5;
                B[k][i + 5] = temp6;
                B[k][i + 6] = temp7;
                B[k][i + 7] = temp8;
            }
        }
    }
    // 尝试 4 * 4 的方式
    // for(int i = 0 ; i < N ; i += 4) {
    //     for(int j = 0 ; j < M ; j += 4) {
    //         for(int k = j ; k < j + 4 ; k ++) {
    //             temp1 = A[i][k];
    //             temp2 = A[i + 1][k];
    //             temp3 = A[i + 2][k];
    //             temp4 = A[i + 3][k];
    //             // 交换位置
    //             B[k][i] = temp1;
    //             B[k][i + 1] = temp2;
    //             B[k][i + 2] = temp3;
    //             B[k][i + 3] = temp4;
    //         }
    //     }
    // }

}



/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 
char trans_desc_handle64[] = "Transpose submission";
void hanlder_64(int M, int N, int A[N][M], int B[M][N])
{
    // 思路: 这里每一次 load 或者 store 的时候只可以利用 4 * 4 的方式,否则就会导致冲突
    // 可以把这些块划分为 多个 8 * 8 的块,在多个 8 * 8 的块的中间进行操作
    int temp1 , temp2 , temp3 , temp4 , temp5 , temp6 , temp7 , temp8;
    int x,y;
    for(int i = 0 ; i < N ; i += 8) {
        for(int j = 0 ; j < M ; j += 8) {
            // 1. 把 A 的左上逆转到 B 的左上 , 并且存储 A 的右边上面的逆转结果
            // 工作范围 i - (i + 4) , j - (j + 8)
            // 总是遵循 列优先的原则
            x = i;
            y = j;
            for( ; x < i + 4 ; x ++) {
                // 存储左上
                temp1 = A[x][y];
                temp2 = A[x][y + 1];
                temp3 = A[x][y + 2];
                temp4 = A[x][y + 3];
                // 存储右上
                temp5 = A[x][y + 4];
                temp6 = A[x][y + 5];
                temp7 = A[x][y + 6];
                temp8 = A[x][y + 7];
                // 翻转左上
                B[y][x] = temp1;
                B[y + 1][x] = temp2;
                B[y + 2][x] = temp3;
                B[y + 3][x] = temp4;
                // 保存右边上 (x , y) -> (y , x) -> (y - 4 , x + 4)
                B[y][x + 4] = temp5;
                B[y + 1][x + 4] = temp6;
                B[y + 2][x + 4] = temp7;
                B[y + 3][x + 4] = temp8;
            }
        // 2. 把 A 的左下移动到 B 的右上,并且把 B 的右上移动到 B 的左下
        // 工作范围: (i + 4) - (i + 8) , j - (j + 4)
        x = i + 4;
        y = j;
        // 采用列优先的方式
        for( ; y < j + 4 ; y ++) {
            // 记录左下
            temp1 = A[x][y];
            temp2 = A[x + 1][y];
            temp3 = A[x + 2][y];
            temp4 = A[x + 3][y];
            // 记录 B 的右上
            temp5 = B[y][x];
            temp6 = B[y][x + 1];
            temp7 = B[y][x + 2];
            temp8 = B[y][x + 3];
            // 进行翻转
            B[y][x] = temp1;
            B[y][x + 1] = temp2;
            B[y][x + 2] = temp3;
            B[y][x + 3] = temp4;
            // 保存信息到 B 的左下
            B[y + 4][x - 4] = temp5;
            B[y + 4][x - 3] = temp6;
            B[y + 4][x - 2] = temp7;
            B[y + 4][x - 1] = temp8;
        }
        // 3. 把 A 的右边下面翻转到 B 的右下
        // 工作范围 (i + 4) - (i + 8) , (j + 4) - (j + 8)
        x = i + 4;
        y = j + 4;
        for( ; x < i + 8 ; x ++) {
            temp1 = A[x][y];
            temp2 = A[x][y + 1];
            temp3 = A[x][y + 2];
            temp4 = A[x][y + 3];

            B[y][x] = temp1;
            B[y + 1][x] = temp2;
            B[y + 2][x] = temp3;
            B[y + 3][x] = temp4;
        }
    }
    
    } 
}

char trans_desc_handle_odd[] = "Transpose submission";
void hanlder_odd(int M, int N, int A[N][M], int B[M][N])
{
    // 利用 8 * 8 的方块进行转置操作
    // N = 67 , M = 61, 需要反过来
    // M = 61 , N = 67
    // 67 * 61
    if(M == 32 && N == 32) {
        handler_32(M , N , A , B);
        return ;
    } else if(M == 64 && N == 64) {
        hanlder_64(M , N , A , B);
        return ;
    }
    int temp1 , temp2 , temp3 , temp4 , temp5 , temp6 , temp7 , temp8;
    int i , j;
    for(i = 0 ; i < 64 ; i += 8) {
        for(j = 0 ; j < 56 ; j += 8) {
            for(int k = j ; k < j + 8 ; k ++) {
                temp1 = A[i][k];
                temp2 = A[i + 1][k];
                temp3 = A[i + 2][k];
                temp4 = A[i + 3][k];
                temp5 = A[i + 4][k];
                temp6 = A[i + 5][k];
                temp7 = A[i + 6][k];
                temp8 = A[i + 7][k];
                // 进行逆转操作
                B[k][i] = temp1;
                B[k][i + 1] = temp2;
                B[k][i + 2] = temp3;
                B[k][i + 3] = temp4;
                B[k][i + 4] = temp5;
                B[k][i + 5] = temp6;
                B[k][i + 6] = temp7;
                B[k][i + 7] = temp8;
            }
        }
    }
    // 可以处理剩下的部分了
    // 首先处理 3 * 56 的矩阵,可以分为 3 * 8 的方块
    for(i = 64 ; i < 67 ; i += 3) {
        for(j = 0 ; j < 56 ; j += 8) {
            for(int k = j ; k < j + 8 ; k ++) {
               temp1 = A[i][k];
               temp2 = A[i + 1][k];
               temp3 = A[i + 2][k];
               // 逆转
               B[k][i] = temp1;
               B[k][i + 1] = temp2;
               B[k][i + 2] = temp3;
            }
        }
    }
    // 最后处理 64 * 5 的方格,可以首先分为 5 * 64 的方格,采用 5 * 8 的方式进行分割
    for(i = 0 ; i < 64 ; i += 8) {
        for(j = 56 ; j < 61 ; j += 5) {
            for(int k = i ; k < i + 8 ; k ++) {
                temp1 = A[k][j];
                temp2 = A[k][j + 1];
                temp3 = A[k][j + 2];
                temp4 = A[k][j + 3];
                temp5 = A[k][j + 4];
                // 逆转
                B[j][k] = temp1;
                B[j + 1][k] = temp2;
                B[j + 2][k] = temp3;
                B[j + 3][k] = temp4;
                B[j + 4][k] = temp5;
            }
        }
    }
    // 最后处理 3 * 5 的方块即可
    // 直接划分成 3 * 5 即可
    for(i = 64 ; i < 67 ; i += 3) {
        for(j = 56 ; j < 61 ; j += 5) {
            for(int k = j ; k < j + 5 ; k ++) {
                temp1 = A[i][k];
                temp2 = A[i + 1][k];
                temp3 = A[i + 2][k];
                // 逆转
                B[k][i] = temp1;
                B[k][i + 1] = temp2;
                B[k][i + 2] = temp3;
            }
        }
    }
}
/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(hanlder_odd , trans_desc_handle_odd); 
    // registerTransFunction(hanlder_64 , trans_desc_handle64); 
    // registerTransFunction(handler_32 , trans_desc_handle32); 

    /* Register any additional transpose functions */
    // registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

