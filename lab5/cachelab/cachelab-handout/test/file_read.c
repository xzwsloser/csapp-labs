#include<stdio.h>
#include<string.h>
#include<stdlib.h>
void read_line(FILE* fp) {
    char buf[100];
    int i = 0;
    char ch;
    ch = fgetc(fp);
    while(ch != EOF && ch != '\n') {
        buf[i] = ch;
        i ++;
        ch = fgetc(fp);
    }
    printf("%s\n",buf);
}
int main() {
    char* file_name = "hello.txt";
    FILE* fp = fopen(file_name , "r");
    read_line(fp);
    read_line(fp);
    read_line(fp);
    read_line(fp);
    read_line(fp);
    read_line(fp);
    read_line(fp);
    read_line(fp);
    read_line(fp);
    char* hello = "1234";
    int c = atoi(hello);
    printf("%d \n" , c);
    fclose(fp);
    return 0;
}