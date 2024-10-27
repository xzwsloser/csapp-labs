#include<string.h>
#include<stdio.h>
int main() {
    char line[1024] = "http://localhost:10086/home.html";
    char host[100];
    char value[100];
    char re[100];
    sscanf(line , "%*[^/]\/\/%*[^/]%s" , re);
    // sscanf(line , "%s\:%s\/%s" , host , value , re);
    // printf("host = %s , value = %s \n" , host , value);
    // 解析
    char new_line[100] = "Host: localhost:1001";
    char opt[100];
    sscanf(new_line , "%[^:]: %s" , opt , host);
    printf("host =%s \n" , host);
    printf("opt =%s\n" , opt);
    char port[10];
    char hostName[100];
    sscanf(host , "%[^:]: %s" , hostName , port);
    printf("hostName =%s , port=%s\n" , hostName , port);
    printf("resource = %s \n" , re);
}