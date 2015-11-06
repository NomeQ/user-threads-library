/* 
CS 533 Class Project
Naomi Dickerson
*/
#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

// Prime function courtesy of Kendall Stewart
void print_nth_prime(void * pn) {
    int n = *(int *) pn;
    int c = 1, i = 1;
    while(c <= n) {
        ++i;
        int j, isprime = 1;
        for(j = 2; j < i; ++j) {
            if(i % j == 0) {
                isprime = 0;
                break;
            }
        }
        if(isprime) {
            ++c;
        }
        yield();
    }
    printf("%dth prime: %d\n", n, i);
}

void input_wait(void * chars) {
    int n = *(int *) chars;
    char * buffer;
    buffer = (char *) malloc(n * sizeof(char));
    printf("Type %d chars: \n", n);
    read_wrap(STDIN_FILENO, buffer, n);
    int i;
    printf("Buffer contents: ");
    for (i = 0; i < n; i++) {
        printf("%c ", buffer[i]);
    }
    free(buffer);    
}

void printBuffer(char * buffer, int n) {
    int i;
    for (i = 0; i < n; i++) {
        printf("%c", buffer[i]);
    }
}

void open_file(void * junk) {
    int fd, ret;
    char buff[10];
    fd = open("test.txt", O_RDONLY);
    printf("Reading 1st 10 chars from test.txt...\n");
    ret = read_wrap(fd, buff, 10);
    printf("Buffer contents: ");
    printBuffer(buff, 10);
    printf("\n");
    printf("return value: %d\n", ret);
    printf("Next 10...\n");
    ret = read_wrap(fd, buff, 10);
    printf("Buffer contents: ");
    printBuffer(buff, 10);
    printf("\n");
    printf("return value: %d\n", ret);
    close(fd);
}

void go1(void) {
    int n1 = 20000, n2 = 10000, n3 = 30000, n4 = 2;
    thread_fork(print_nth_prime, &n1);
    thread_fork(print_nth_prime, &n2);
    thread_fork(print_nth_prime, &n3);
    thread_fork(open_file, &n4);
}

int main(void) {
    scheduler_begin();
  
    go1();

    scheduler_end();
    return 0;
}
  
