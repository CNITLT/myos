#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "stdint.h"
#include "bitmap.h"
#include "e820.h"
#include "mutex.h"
#include "list.h"
#define ALIGN_SIZE 4
#define ALIGN(x,size) ((x+size - 1) & ~(size-1))
#define ALIGN_DOWN(x,size) (x & ~(size-1))
//定义7个管理小Block的描述符 从16，32直到1024
#define BLOCK_DESC_SIZE 7
#define BLOCK_MIN_SIZE 16
#define BLOCK_MAX_SIZE (BLOCK_MIN_SIZE << (BLOCK_DESC_SIZE - 1))
struct mem_block {
    struct list_node free_node;
};

struct mem_block_desc{
    //这个结构是被arena共享的
    size_t block_size; //块大小
    size_t blocks_per_arena; //这里的arena基本上就代表一个页的概念，等价于是一个页去掉一些存元信息的空间，内部的blocks有多少块
    struct list free_list;
};

struct arena{
    // 如果是小于等于1024的内存大小这个才有用, 超过1024的被认为是大空间，按所需页直接分配，不采用小block的分配方法
    struct mem_block_desc* p_block_desc;
    union  {
        size_t free_count;//large_flag为False，则这个有用 表明空闲块的数目 则这个arena只是管理一页大小的空间
        size_t page_count;//large_flag为True, 则这个有用 表明这个arena管理的是多页分配，这里表明分配了多少页
    } count;
    bool large_flag;//采用懒加载的方式，所以不能按p_desc是不是null来判断，可能是还有请求到达，没进行初始化
};


typedef struct memory_pool{
    size_t start;//内存起点
    size_t length; //长度单位字节
    size_t used;//已分配字节数
    struct mutex lock;
    bitmap bmap;
} memory_pool;

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
@brief 初始化用户虚拟内存池，会自动malloc出一定的内核页存bitmap
@param p_user_vmemory_pool: memory_pool* :内存池指针
*/
void user_vmemory_pool_init(memory_pool* p_user_vmemory_pool);

/*
@brief 有关内存的初始化汇总。
*/
void memory_init();
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
@param page_attr:uint32_t: 页属性
@return vaddr_t 分配后的页虚拟首地址，分配失败返回NULL
*/
vaddr_t malloc_page_core(vaddr_t start_vaddr, size_t page_count, memory_pool* p_vmemory_pool, vaddr_t page_dir, uint32_t page_attr);

/*
@brief 从虚拟内存池和物理内存池释放vaddr对应的页为起点开始的page_count个页，并解除页目录和页表里的映射关系
@param vaddr:vaddr_t:虚拟地址
@param page_count: size_t: 释放页的数量
@param p_vmemory_pool:memory_pool*: 对应的虚拟内存池地址
@param page_dir:vaddr_t: 页目录地址
*/
void free_page_core(vaddr_t vaddr, size_t page_count,  memory_pool* p_vmemory_pool, vaddr_t page_dir);


/*
@brief 专门为内核分配页的malloc_page函数
@param page_count:size_t: 需要分配页的数目
@return vaddr_t 分配后的页虚拟首地址，分配失败返回NULL
*/
vaddr_t malloc_kernel_page(size_t page_count);


/*
@brief 专门为释放内核页的free_page函数
@param vaddr_t:vaddr_t: 起点页内的任意虚拟内存地址
@param page_count:size_t: 从vaddr_t开始的页计数，要释放的页数目
*/
void free_kernel_page(vaddr_t vaddr, size_t page_count);


/*
@brief 为用户线程分配页的malloc_page函数, 要求当前PCB得是用户的
@param page_count:size_t: 需要分配页的数目
@return vaddr_t 分配后的页虚拟首地址，分配失败返回NULL
*/
vaddr_t malloc_user_page(size_t page_count);

/*
@brief 专门为释放用户的free_page函数
@param vaddr_t:vaddr_t: 起点页内的任意虚拟内存地址
@param page_count:size_t: 从vaddr_t开始的页计数，要释放的页数目
*/
void free_user_page(vaddr_t vaddr, size_t page_count);


/*
@brief 为当前线程分配页的malloc_page函数 自动区分内核线程和用户线程 该函数时运行必须在PCB的栈内 
@param page_count:size_t: 需要分配页的数目
@return vaddr_t 分配后的页虚拟首地址，分配失败返回NULL
*/
vaddr_t malloc_page(size_t page_count);

/*
@brief 释放页的函数，自动区分内核线程和用户线程
@param vaddr_t:vaddr_t: 起点页内的任意虚拟内存地址
@param page_count:size_t: 从vaddr_t开始的页计数，要释放的页数目
*/
void free_page(vaddr_t vaddr, size_t page_count);



/*
@brief 初始化block_desc数组
@param desc_array: struct mem_block_desc*: 数组首地址
*/
void mem_block_desc_array_init(struct mem_block_desc* desc_array);

/*
@brief 获取arena内索引位对应的block, 若不是按block管理，则默认返回可用内存的首地址
@param p_arena: struct arena*: arena首地址
@param index: size_t : block 索引从0开始
@return struct mem_block*: 可用内存的首地址，超出index范围返回NULL, 若不是按block管理，则默认返回可用内存的首地址
*/
struct mem_block* arena2block(struct arena* p_arena, size_t index);

/*
@brief 返回block所在的arena地址，只能对管理block的arena分配出去的地址使用
@param p_block: struct mem_block* :管理block的arena分配出去的地址
@return struct arena*: 管理此block的arena地址
*/
struct arena* block2arena(struct mem_block* p_block);

/*
@brief 从内存池里面分配一定内存并初始化为以页为单位管理的arena 调用时要求是内核态
@param page_count: size_t : 所需要分配的页数
@return struct arena*: 所分配的arena首地址, 失败返回NULL
*/
struct arena* malloc_and_init_page_arena(size_t page_count);

/*
@brief 从内存池里面分配一页内存并初始化为以block为单位管理的arena 调用时要求是内核态
@param p_block_desc: struct mem_block_desc* : 对应的block_desc的地址
@return struct arena*: 所分配的arena首地址, 失败返回NULL
*/
struct arena* malloc_and_init_block_arena(struct mem_block_desc* p_block_desc);

/*
@brief 分配指定大小的内存空间
@param size: size_t :字节为单位的空间大小
@return void *:可用空间的首地址
*/
void *sys_malloc(size_t size);


/*
@brief 释放sys_malloc分配的空间
@param p: void * :将要释放的sys_malloc分配的地址
*/
void sys_free(void* p);


/*
@brief 分配指定大小的内存空间在内核上,本质还是sys_malloc
@param size: size_t :字节为单位的空间大小
@return void *:可用空间的首地址
*/
void *sys_malloc_in_kernel(size_t size);


/*
@brief 释放sys_malloc分配的在内核上的空间,本质还是sys_free
@param p: void * :将要释放的sys_malloc分配的地址
*/
void sys_free_in_kernel(void* p);

#endif