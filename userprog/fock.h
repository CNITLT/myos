#ifndef __USERPROG_FOCK_H
#define __USERPROG_FOCK_H

#include "stdint.h"
#include "stddef.h"
struct task_struct;

/*
 * @brief 复制父进程的PCB并给子进程，并修改部分参数
 * @param child_pcb: struct task_struct *: 子进程pcb地址，非空
 * @param parent_pcb: struct task_struct *: 父进程pcb地址，非空
*/
void copy_parent_pcb_to_child(struct task_struct *child_pcb, struct task_struct *parent_pcb);


#endif