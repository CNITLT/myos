#ifndef __USERPROG_EXEC_H
#define __USERPROG_EXEC_H

#include "stdint.h"
#include "thread.h"
#include "elf32.h"

/*
 * @brief 执行一个新的程序，替换当前进程的内存映像
 * @param path: 程序路径
 * @param argv: 参数数组
 * @return 成功返回0，失败返回-1
 */
int sys_exec(const char* path, char* const argv[]);

/*
 * @brief 将文件描述符指向的文件偏移为offset大小的数据加载到虚拟地址vaddr, 使用前的一开始的用户进程空间基本上应该是空的，或者说要加载段的地方没有被分配
 * @param new_pcb: 新的进程控制块
 * @param fd: 文件描述符
 * @param offset: 文件偏移
 * @param filesz: 文件大小
 * @param vaddr: 虚拟地址
 * @return 成功返回true，失败返回false
*/
bool segment_load(struct task_struct *new_pcb, int32_t fd, uint32_t offset, uint32_t filesz, uint32_t vaddr);

/*
 * @brief 加载可执行文件，但不执行，返回加载后的进程控制块
 * @param path: 可执行文件路径
 * @return 成功返回进程控制块指针，失败返回NULL
*/
struct task_struct *sys_load(const char * path);
#endif