#ifndef __SYSCALL_INIT_H
#define __SYSCALL_INIT_H
#include "stdint.h"
#include "stddef.h"
#include "thread.h"
//系统调用个数
#define SYSCALL_SIZE 64
//用uint32_t其实只是为了与寄存器大小一致 实际函数还是要看具体的系统调用类别
typedef uint32_t syscall_ret_type;
typedef uint32_t syscall_param_type; 
typedef void* syscall_addr;
// 最多只用三个参数的系统调用,这个是统一形式上的调用方式
typedef syscall_ret_type (*syscall_call_proxy)(syscall_param_type syscallNum,...);
extern syscall_addr syscall_table[SYSCALL_SIZE];

/*
@brief 初始化系统调用
*/
void syscall_init();


//sys开头的函数都是系统调用对应的实际执行功能的函数
/*
@brief 返回当前线程的PID
@return pid_t 线程PID
*/
pid_t sys_getpid(void);

#endif