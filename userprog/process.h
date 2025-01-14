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
#endif