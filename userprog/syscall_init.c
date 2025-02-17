#include "syscall_init.h"
#include "print.h"
#include "syscall.h"
#include "memory.h"
syscall_addr syscall_table[SYSCALL_SIZE] = {NULL};
/*
默认系统调用中断，还是只打印一句话
*/
static void syscall_default_func(){
    printf("this is default syscall func\n");
}

void syscall_init(){
    for(int i = 0; i < SYSCALL_SIZE;i++){
        syscall_table[i] = syscall_default_func;
    }
    // TODO:: 有需要这里加额外的系统调用中断
    syscall_table[SYS_GETPID] = sys_getpid;
    syscall_table[SYS_MALLOC] = sys_malloc;
    syscall_table[SYS_FREE] = sys_free;
}


pid_t sys_getpid(void){
    return get_current_pcb()->pid;
}