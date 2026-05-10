#include "test_thread.h"
#include "thread.h"
#include "syscall.h"

void test_fork() {
    pid_t pid = fork();
    if (pid) {
        printf("this is parent process fork ret:%d intr_state:%d\n", pid, get_interrupt_state());
    }  else {
        printf("this is child process fork ret:%d intr_state:%d\n", pid, get_interrupt_state());
    }
    while(1);
}