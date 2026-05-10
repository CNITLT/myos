#ifndef __USERPROG_FORK_H
#define __USERPROG_FORK_H

#include "stdint.h"
#include "stddef.h"
struct task_struct;

/*
 * @brief 复制父进程的PCB并给子进程，并修改部分参数
 * @param child_pcb: struct task_struct *: 子进程pcb地址，非空
 * @param parent_pcb: struct task_struct *: 父进程pcb地址，非空, 且为用户线程
*/
void copy_parent_pcb_to_child(struct task_struct *child_pcb, struct task_struct *parent_pcb);

/*
 * @brief 复制父进程的用户进程内的数据给子进程，并修改对应的页表项映射，如果父进程是内核进程，则该函数什么都不做
 * @param child_pcb: struct task_struct *: 子进程pcb地址，非空
 * @param parent_pcb: struct task_struct *: 父进程pcb地址，非空, 且为用户线程
*/
void copy_parent_user_sapce_data_to_child(struct task_struct *child_pcb, struct task_struct *parent_pcb);

/*
 * @brief 调整刚刚的复制的子进程PCB的栈，用于确定返回栈的构件和返回地址的构件
 * 
*/
void adjust_copyed_child_pcb_stack(struct task_struct *child_pcb);

/*
 * @brief 复制父进程信息并调整子进程PCB与页表等信息，等价于是多个步骤的汇总
 * @param child_pcb: struct task_struct *: 子进程pcb地址，非空
 * @param parent_pcb: struct task_struct *: 父进程pcb地址，非空, 且为用户线程
*/
void copy_process(struct task_struct *child_pcb, struct task_struct *parent_pcb);

/*
 * @brief 复制一个子进程，父进程返回子进程pid, 子进程返回0， 失败返回-1
*/
pid_t sys_fork();
#endif