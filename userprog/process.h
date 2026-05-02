#ifndef __PROCESS_H
#define __PROCESS_H
#include "thread.h"

#define USER_PROCESS_DEFAULT_PRIOR 31
// 1GB内核 3GB用户空间，且用户高地址的一部分还留出一部分用来存其他数据
#define USER_STACK3_VADDR  (0xc0000000 - 0x1000)
// 根据linux来的 0x8048000以下的地址保留不用，用户进程从这里开始分地址
#define USER_VADDR_START 0x8048000

// 用户空间的最大长度
#define USER_PROCESS_MEMORY_MAX_LENGTH (0xC0000000 - USER_VADDR_START)

/*
@brief 用户态相关的GDT表项目初始化
*/
void user_gdt_init();

/*
@brief 用户进程环境构建函数，构件一个用户进程的上下文初始信息，通过switch_to切换到kernel_thread函数，再被调用，初始时候只有一个PCB可用， 但虚拟地址空间池和页目录已创建
@param filename: void * :目前只是当函数地址用
@TODO 后续还要修改
@note 不会通过这个函数返回
*/
void start_process(void* filename);

/*
@brief 执行一个用户进程, 特权级3
@param filename: void * :目前是当函数地址使用
@param name: char * : 进程名
@return struct task_struct*: 进程PCB地址
*/
struct task_struct* process_execute(void* filename, char* name);


/*
@brief 创建一个用户进程的页目录，复制1GB内核对应的页表并初始化3GB的用户页表
@return vaddr_t: 页目录的虚拟地址
*/
vaddr_t create_page_dir(void);


/*
@brief 激活指定PCB对应的进程,主要目的是替换页表和TSS的对应的0级中断栈
@param pcb: struct task_struct* :对应的PCB地址
*/
void process_activate(struct task_struct* pcb);

/*
@brief 激活指定PCB对应的页表
@param pcb: struct task_struct* :对应的PCB地址
*/
void page_dir_activate(struct task_struct* pcb);
#endif