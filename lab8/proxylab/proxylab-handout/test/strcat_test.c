#include<stdio.h>
#include<string.h>
int main() {
    char buf[100];
    char b[20] = "hello";
    for(int i = 0 ; i < 4 ; i ++) {
        strcat(buf , b);
    }
    printf("%s\n" , buf);
}