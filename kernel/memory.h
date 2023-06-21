#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "stdint.h"
#include "bitmap.h"
#include "e820.h"
#define ALIGN_SIZE 4
#define ALIGN(x,size) ((x+size - 1) & ~(size-1))
#define ALIGN_DOWN(x,size) (x & ~(size-1))
#define KERNEL_HEAP_START_VADDR 0xC0800000

typedef struct memory_pool{
    size_t start;//内存起点
    size_t length; //长度单位字节
    size_t used;//已分配字节数
    bitmap bmap;
}memory_pool;
extern memory_pool kernel_vmemory_pool;
/*
@brief 在栈上分配空间
@param size:size_t: 需要分配的内存大小
@return void* 分配后空间首地址
*/
void* __alloca(size_t size);

/*
@brief 从指定的内存池分配内存页
@param page_count:size_t: 分配的页数
@param p_memory_pool:memory_pool*: 内存池地址
@param start_index:size_t: 分配的查找起点
@return addr_t: 分配后的内存页首地址，分配失败返回NULL
*/
addr_t malloc_page_from_pool(size_t page_count,memory_pool* p_memory_pool, size_t start_index);

/*
@brief 从指定内存池释放从addr对应的页为起点开始的page_count个页，推荐用4K对齐的地址，不对齐也行
@param addr:addr_t: 页内的任意内存，但推荐4K对齐的首地址
@param page_count:size_t: 共释放多少页
@param p_memory_pool:memory_pool*: 内存池地址
*/
void free_page_from_pool(addr_t addr, size_t page_count,memory_pool* p_memory_pool);

/*
@brief 初始化物理内存池
*/
void pmemory_pool_init();


/*
@brief 初始化内核虚拟内存池
*/
void kernel_vmemory_pool_init();

/*
@brief 物理内存池和内核虚拟内存池的初始化汇总。
*/
void memory_pool_init();
/*
@brief 从物理内存池分配内存页
@param page_count:size_t:分配的页数
@return paddr_t: 分配后的内存页物理首地址，分配失败返回NULL
*/
paddr_t pmalloc_page(size_t page_count);


/*
@brief 释放paddr对应的物理页为起点开始的page_count个页，推荐用4K对齐的地址，不对齐也行
@param paddr:paddr_t: 物理页内的任意内存，但推荐4K对齐的首地址
@param page_count: 共释放多少物理页
*/
void pfree_page(paddr_t paddr, size_t page_count);

/*
@brief 分配指定数量的物理内存页和虚拟内存页并建立映射关系
@param start_vaddr:vaddr_t: 虚拟地址的分配起点, 传入堆的起点就行
@param page_count:size_t: 分配的页数量
@param p_vmemory_pool:memory_pool*:进程对应的虚拟内存池
@param page_dir:vaddr_t: 进程对应的页目录虚拟地址
@return vaddr_t 分配后的页虚拟首地址，分配失败返回NULL
*/
vaddr_t malloc_page(vaddr_t start_vaddr, size_t page_count, memory_pool* p_vmemory_pool, vaddr_t page_dir);

/*
@brief 从虚拟内存池和物理内存池释放vaddr对应的页为起点开始的page_count个页，并解除页目录和页表里的映射关系
@param vaddr:vaddr_t:虚拟地址
@param page_count: size_t: 释放页的数量
@param p_vmemory_pool:memory_pool*: 对应的虚拟内存池地址
@param page_dir:vaddr_t: 页目录地址
*/
void free_page(vaddr_t vaddr, size_t page_count,  memory_pool* p_vmemory_pool, vaddr_t page_dir);


#endif