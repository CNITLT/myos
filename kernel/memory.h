#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "stdint.h"
#define ALIGN_SIZE 4
#define ALIGN(x,size) ((x+size - 1) & ~(size-1))
#define ALIGN_DOWN(x,size) (x & ~(size-1))




/*
@brief 在栈上分配空间
@param size:size_t: 需要分配的内存大小
@return void* 分配后空间首地址
*/
void* __alloca(size_t size);


#endif