#include<stdio.h>
#include<string.h>
#include<stdlib.h>

char* func();
int main()
{
    char* ptr = func();
    printf("%s\n" , ptr);
}
char* func()
{
    char* p = (char*)malloc(sizeof(char)*10);
    strcpy(p , "hello");
    char* ptr = p;
    return ptr;
}

