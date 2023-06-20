#include "memory.h"
#include "string.h"
#include "stddef.h"
#include "debug.h"
#include "e820.h"
#include "page.h"
//1MB内存的地方可用,用来存放bitmap
#define MEMORY_POOL_BITMAP_ADDR 0x100000 

static memory_pool ph_memory_pool;

void* __alloca(size_t size){
    assert(size != 0);
    //调用函数的时候看了反汇编是先sub $12,%%esp, 再push,用16字节对齐了栈，等价于传了16字节的参数，但也看了下其他的函数调用，又没对齐栈
    //所以这里返回的地址用16字节对齐，再加16字节缓冲区来保证不错，虽然空间浪费有点严重
    void *ebp = NULL;
    void *esp = NULL;
    asm volatile("\
    movl %%ebp, %0;\
    movl %%esp, %1;\
    ":"=m"(ebp),"=m"(esp):\
    );

    size = ALIGN(size, 4);
    void *ret = (void *)(ALIGN_DOWN(((uint32_t)(ebp) - size),16));// 以ebp为准，分配对齐后的size大小空间，再以对齐后的地址为返回值

  
    asm volatile("\
    movl %0, %%esp;\
    subl $16, %%esp;\
    subl $24, %%esp;\
    movl 8(%%ebp), %%edx;\
    movl %%edx, 8(%%esp);\
    movl 4(%%ebp), %%edx;\
    movl %%edx, 4(%%esp);\
    movl (%%ebp), %%edx;\
    movl %%edx, (%%esp);\
    "::"a"(ret));//先减16当缓冲区，24拆分为12 + 12， 12是size eip ebp三个4字节的栈数据，后面构造一个一样的栈，从这里返回，防止空间被回收, 
    //另一个12用于对齐, 主要对齐参考是参数size的地址要16对齐，后面的EIP,EBP无所谓

    asm volatile("\
    pop %%ebp;\
    ret;\
    "::"a"(ret)
    );//这里才是真的返回，要改ebp，不能从开始的ebp返回，不然栈空间在函数结束后会被回收，分配失败，所以不用return
    return ret;//不会走这里返回，写这个是为了关掉提示
}

addr_t malloc_page(memory_pool* p_memory_pool, size_t page_count){
    size_t index = bitmap_find_range(&p_memory_pool->bmap, page_count);
    if(index == BITMAP_RANGE_NOTFOUND){
        return NULL;
    }
    bitmap_range_set(&p_memory_pool->bmap, index, page_count, BIT_STATE_USE);
    p_memory_pool->used += PAGE_SIZE * page_count;
    return (addr_t)(PAGE_SIZE * index + p_memory_pool->start);  
}


void free_page(memory_pool* p_memory_pool, addr_t addr, size_t page_count){
    size_t index = ((size_t)addr - p_memory_pool->start) / PAGE_SIZE / 8;
    bitmap_range_set(&p_memory_pool->bmap, index, page_count, BIT_STATE_UNUSE); 
    p_memory_pool->used -= PAGE_SIZE * page_count; 
}

void pmemeory_pool_init(){
    //1MB以下的不管，默认是分给内核的，只管理1MB之上的
    e820_entry max_useble;
    get_max_memory_e820_entry(&max_useble);
    ph_memory_pool.start = max_useble.addr;
    ph_memory_pool.length = max_useble.length;
    ph_memory_pool.bmap.len_bit = max_useble.length/PAGE_SIZE;
    ph_memory_pool.bmap.bits = (uint8_t *)MEMORY_POOL_BITMAP_ADDR;
    bitmap_init(&ph_memory_pool.bmap);
    //设置已经分配过的内存 1- 24MB范围的内存设置为已经分配
    bitmap_range_set(&ph_memory_pool.bmap,(0x100000-ph_memory_pool.start)/PAGE_SIZE,0x2300000/PAGE_SIZE, BIT_STATE_USE);
    ph_memory_pool.used = 0x2300000;
}

paddr_t pmalloc_page(size_t page_count){
    return malloc_page(&ph_memory_pool, page_count);
}

void pfree_page(paddr_t paddr, size_t page_count){
    free_page(&ph_memory_pool, paddr, page_count);
}
