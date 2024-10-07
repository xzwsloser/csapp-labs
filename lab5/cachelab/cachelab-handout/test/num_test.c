#include<stdio.h>
int main() {
    unsigned a = 0xFFFFFFFF;
    int b = 0xF0FD1D21; // 0xF0
    int res = (b >> 8) & (a >> 20);
    int k = 0xFFF;
    printf("%d \n" , k);
    printf("res = %x \n" , res);
}