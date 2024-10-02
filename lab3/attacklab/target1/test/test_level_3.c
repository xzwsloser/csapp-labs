#include<stdio.h>
#include<string.h>
int main() {
    char buf[100];
    unsigned val = 0xfffffff888;
    sprintf(buf , "%.8x" , val);
    printf("%s\n" , buf);
    char* str = "fffffff899";
    printf("%d \n" , strncmp(str , buf , 9));
}