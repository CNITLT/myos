#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "stdint.h"
#include "bitmap.h"
#include "e820.h"
#define ALIGN_SIZE 4
#define ALIGN(x,size) ((x+size - 1) & ~(size-1))
#define ALIGN_DOWN(x,size) (x & ~(size-1))

typedef struct memory_pool{
    size_t start;//内存起点
    size_t length; //长度单位字节
    size_t used;//已分配字节数
    bitmap bmap;
}memory_pool;

/*
@brief 在栈上分配空间
@param size:size_t: 需要分配的内存大小
@return void* 分配后空间首地址
*/
void* __alloca(size_t size);

/*
@brief 从指定的内存池分配内存页
@param p_memory_pool:memory_pool*: 内存池地址
@param page_count:size_t:分配的页数
@return addr_t: 分配后的内存页首地址，分配失败返回NULL
*/
addr_t malloc_page(memory_pool* p_memory_pool, size_t page_count);

/*
@brief 从指定内存池释放addr对应的物理页，推荐用4K对齐的地址，不对齐也行
@param p_memory_pool:memory_pool*: 内存池地址
@param addr:addr_t: 页内的任意内存，但推荐4K对齐的首地址
@param page_count: 共释放多少页
*/
void free_page(memory_pool* p_memory_pool, addr_t addr, size_t page_count);

/*
@brief 初始化物理内存池
*/
void pmemeory_pool_init();

/*
@brief 从物理内存池分配内存页
@param page_count:size_t:分配的页数
@return paddr_t: 分配后的内存页物理首地址，分配失败返回NULL
*/
paddr_t pmalloc_page(size_t page_count);


/*
@brief 释放paddr对应的物理页，推荐用4K对齐的地址，不对齐也行
@param paddr:paddr_t: 物理页内的任意内存，但推荐4K对齐的首地址
@param page_count: 共释放多少物理页
*/
void pfree_page(paddr_t paddr, size_t page_count);

#endif