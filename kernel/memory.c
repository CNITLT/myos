#include "memory.h"
#include "string.h"
#include "stddef.h"
#include "debug.h"
#include "e820.h"
#include "page.h"
//1MB内存的地方可用,用来存放bitmap
#define PHMEMORY_POOL_BITMAP_ADDR 0x100000 
//内核虚拟内存池bitmap地址, 物理内存池在管理4GB的时候长度最大，需要的bitmap最大长度是128kb
#define KERNEL_VMEMORY_POOL_BITMAP_ADDR (PHMEMORY_POOL_BITMAP_ADDR + 128 * 1024)



static memory_pool ph_memory_pool;
memory_pool kernel_vmemory_pool;
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

addr_t malloc_page_from_pool(size_t page_count,memory_pool* p_memory_pool, size_t start_index){
    size_t index = bitmap_find_range_from_index(&p_memory_pool->bmap, page_count, start_index);
    if(index == BITMAP_RANGE_NOTFOUND){
        return NULL;
    }
    bitmap_range_set(&p_memory_pool->bmap, index, page_count, BIT_STATE_USE);
    p_memory_pool->used += PAGE_SIZE * page_count;
    return (addr_t)(PAGE_SIZE * index + p_memory_pool->start);  
}


void free_page_from_pool(addr_t addr, size_t page_count,memory_pool* p_memory_pool){    
    size_t index = ((uintaddr_t)addr - p_memory_pool->start) / PAGE_SIZE;
    bitmap_range_set(&p_memory_pool->bmap, index, page_count, BIT_STATE_UNUSE); 
    p_memory_pool->used -= PAGE_SIZE * page_count; 
}

void pmemory_pool_init(){
    //todo:: 随内存布局的改变要改变
    //1MB以下的不管，默认是分给内核的，只管理1MB之上的
    e820_entry max_useble;
    get_max_memory_e820_entry(&max_useble);
    ph_memory_pool.start = max_useble.addr;
    ph_memory_pool.length = max_useble.length;
    ph_memory_pool.bmap.len_bit = max_useble.length/PAGE_SIZE;
    ph_memory_pool.bmap.bits = (uint8_t *)PHMEMORY_POOL_BITMAP_ADDR;
    bitmap_init(&ph_memory_pool.bmap);
    //设置已经分配过的内存 1- 24MB范围的内存设置为已经分配
    bitmap_range_set(&ph_memory_pool.bmap,(0x100000-ph_memory_pool.start)/PAGE_SIZE,0x1700000/PAGE_SIZE, BIT_STATE_USE);
    ph_memory_pool.used = 0x1700000;
}

void memory_pool_init(){
    pmemory_pool_init();
    kernel_vmemory_pool_init();
}

void kernel_vmemory_pool_init(){
    //todo:: 随内存布局的改变要改变
    kernel_vmemory_pool.start = 0xC0000000;//内核起点
    kernel_vmemory_pool.length = 1024*1024*1024;//长度是1GB
    kernel_vmemory_pool.bmap.len_bit = kernel_vmemory_pool.length / PAGE_SIZE;
    kernel_vmemory_pool.bmap.bits = (uint8_t *)KERNEL_VMEMORY_POOL_BITMAP_ADDR;
    bitmap_init(&kernel_vmemory_pool.bmap);    
    //设置已经分配的内存， 1-4MB的直接映射暂时没管
    //内核映像
    bitmap_range_set(&kernel_vmemory_pool.bmap,(0xC0000000-kernel_vmemory_pool.start)/PAGE_SIZE,0x800000/PAGE_SIZE, BIT_STATE_USE);
    //内核栈
    bitmap_range_set(&kernel_vmemory_pool.bmap,(0xFB800000-kernel_vmemory_pool.start)/PAGE_SIZE,0x800000/PAGE_SIZE, BIT_STATE_USE);
    //页目录和页表
    bitmap_range_set(&kernel_vmemory_pool.bmap,(0xFFC00000-kernel_vmemory_pool.start)/PAGE_SIZE,0x400000/PAGE_SIZE, BIT_STATE_USE); 
    kernel_vmemory_pool.used = 0x1400000;   

}

paddr_t pmalloc_page(size_t page_count){
    return malloc_page_from_pool(page_count, &ph_memory_pool,0);
}

void pfree_page(paddr_t paddr, size_t page_count){
    free_page_from_pool(paddr, page_count,&ph_memory_pool);
}


vaddr_t malloc_page(vaddr_t start_vaddr, size_t page_count, memory_pool* p_vmemory_pool, vaddr_t page_dir){
    paddr_t paddr = pmalloc_page(page_count);
    if(paddr == NULL){
        put_str("pmalloc_page faild!\n");
        return NULL;
    }
    vaddr_t vaddr = malloc_page_from_pool(page_count, p_vmemory_pool, (((uintaddr_t)start_vaddr-p_vmemory_pool->start)/PAGE_SIZE));
    
    if(vaddr == NULL){
        pfree_page(paddr, page_count);
        put_str("vmalloc_page faild!\n");
        return NULL;
    }
    //计算可能有多少个不同的页表
    size_t table_count = get_page_dir_entry_vaddr(((uintaddr_t)start_vaddr + (page_count-1) * PAGE_SIZE), page_dir) - 
                        get_page_dir_entry_vaddr(start_vaddr, page_dir) + 1;
    
    paddr_t table_paddr = pmalloc_page(table_count);
    if(table_paddr == NULL){
        pfree_page(paddr, page_count);
        free_page_from_pool(vaddr, page_count, p_vmemory_pool);
        put_str("pmalloc table faild!\n");
        return NULL;
    }
    size_t table_paddr_index = 0;//可以使用的页表需要的物理页索引
    //页都分配完毕，开始映射
    for(size_t i = 0; i < page_count; i++){
        //待映射的虚拟地址
        vaddr_t t_vaddr = (vaddr_t)((uintaddr_t)vaddr + i * PAGE_SIZE);
        //待映射的物理页地址
        paddr_t t_paddr = (paddr_t) ((uintaddr_t)paddr + i * PAGE_SIZE);
        page* p_page_dir_entry = get_page_dir_entry_vaddr(t_vaddr, page_dir);
        if(p_page_dir_entry->P == PAGE_P_VALUE_UNEXIST){
            //说明没有对应的页表，映射一个页表
            set_page_dir_entry(t_vaddr, (paddr_t)((uintaddr_t)table_paddr + PAGE_SIZE * table_paddr_index ), page_dir, PAGE_P_ATTR_EXIST | PAGE_RW_ATTR_RW | PAGE_US_ATTR_SYS);//todo:: 属性这里还需要之后会有用户进程的分配
            table_paddr_index++;
        }
        set_page_table_entry(t_vaddr, t_paddr, page_dir, PAGE_P_ATTR_EXIST | PAGE_RW_ATTR_RW | PAGE_US_ATTR_SYS);
    }
    return vaddr;
}


void free_page(vaddr_t vaddr, size_t page_count,  memory_pool* p_vmemory_pool, vaddr_t page_dir){
    for(size_t i = 0; i < page_count; i++){
        vaddr_t t_vaddr = (vaddr_t)((uintaddr_t)vaddr + i * PAGE_SIZE);
        page* p_page_table_entry = get_page_table_entry_vaddr(t_vaddr, page_dir);
        pfree_page((paddr_t)(p_page_table_entry->PADDR * PAGE_SIZE),1);
        set_page_table_entry(t_vaddr, NULL, page_dir, PAGE_P_ATTR_UNEXIST);
    }
    free_page_from_pool(vaddr, page_count, p_vmemory_pool); 
}