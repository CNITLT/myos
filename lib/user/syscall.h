#ifndef __SYS_CALL_H
#define __SYS_CALL_H
#include "stdint.h"
#include "stddef.h"
//系统调用个数
#define SYSCALL_SIZE 64
//用int32_t其实只是为了与寄存器大小一致 uint32_t其实也一样
typedef int32_t syscall_ret_type;
typedef int32_t syscall_param_type; 
typedef void* syscall_addr;
// 最多只用三个参数的系统调用,这个是统一形式上的调用方式
typedef syscall_ret_type (*syscall_call_proxy)(syscall_param_type syscallNum,...);
extern syscall_addr syscall_table[SYSCALL_SIZE];

/*
@brief 系统调用入口
@param syscallNum: syscall_param_type : 本质就是int 系统调用号
@param ... 可变参数，看系统调用号的不同而不同，目前只支持32位大小，最多3个额外参数
@return syscall_ret_type 本质int32_t 返回值，具体函数看系统调用的不同而不同
*/
syscall_ret_type syscall(syscall_param_type syscallNum, ...);
#endif