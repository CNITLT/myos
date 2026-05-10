#ifndef __USERPROG_FOCK_H
#define __USERPROG_FOCK_H

#include "stdint.h"
#include "stddef.h"
struct task_struct;

/*
 * @brief 复制父进程的PCB并给子进程，并修改部分参数
 * @param child_pcb: struct task_struct *: 子进程pcb地址，非空, 且为用户线程
 * @param parent_pcb: struct task_struct *: 父进程pcb地址，非空
*/
void copy_parent_pcb_to_child(struct task_struct *child_pcb, struct task_struct *parent_pcb);

/*
 * @brief 复制父进程的用户进程内的数据给子进程，并修改对应的页表项映射，如果父进程是内核进程，则该函数什么都不做
 * @param child_pcb: struct task_struct *: 子进程pcb地址，非空, 且为用户线程
 * @param parent_pcb: struct task_struct *: 父进程pcb地址，非空
*/
void copy_parent_user_sapce_data_to_child(struct task_struct *child_pcb, struct task_struct *parent_pcb);

#endif