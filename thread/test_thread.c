#include "test_thread.h"
#include "thread.h"
#include "syscall.h"

void test_fork() {
    pid_t pid = fork();
    if (pid) {
        printf("this is parent process fork ret:%d intr_state:%d now_pid:%d\n", pid, get_interrupt_state(), getpid());
    }  else {
        printf("this is child process fork ret:%d intr_state:%d now_pid:%d\n", pid, get_interrupt_state(), getpid());
    }
    while(1);
}