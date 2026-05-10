#include "test_thread.h"
#include "thread.h"
#include "syscall.h"

void test_fork() {
    pid_t pid = fork();
    if (pid) {
        printf("this is parent process fork ret:%d", pid);
    }  else {
       printf("this is parent process fork ret:%d", pid);
    }
    while(1);
}