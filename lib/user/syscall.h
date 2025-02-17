#ifndef __SYS_CALL_H
#define __SYS_CALL_H
#include "stdint.h"
#include "stddef.h"
#include "syscall_init.h"

enum SYSCALL_NR {
    SYS_GETPID = 0,
    SYS_MALLOC,
    SYS_FREE,
 };
/*
@brief 用户态的系统调用入口
@param syscallNum: syscall_param_type : 本质就是int 系统调用号
@param ... 可变参数，看系统调用号的不同而不同，目前只支持32位大小，最多3个额外参数
@return syscall_ret_type 本质int32_t 返回值，具体函数看系统调用的不同而不同
*/
syscall_ret_type syscall(syscall_param_type syscallNum, ...);

//这里是系统调用的代理，用户态可用

/*
@brief 返回当前线程的PID
@return pid_t 线程PID
*/
pid_t getpid(void);


/*
@brief 分配指定大小的内存空间
@param size: size_t :字节为单位的空间大小
@return void *:可用空间的首地址
*/
void* malloc(size_t size);

/*
@brief 释放malloc分配的空间
@param p: void * :将要释放的sys_malloc分配的地址
*/
void free(void* p);
#endif