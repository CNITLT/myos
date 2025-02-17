#include "memory.h"
#include "string.h"
#include "stddef.h"
#include "debug.h"
#include "e820.h"
#include "page.h"
#include "process.h"
//1MB内存的地方可用,用来存放bitmap
#define PHMEMORY_POOL_BITMAP_ADDR 0x100000 
//内核虚拟内存池bitmap地址, 物理内存池在管理4GB的时候长度最大，需要的bitmap最大长度是128kb
#define KERNEL_VMEMORY_POOL_BITMAP_ADDR (PHMEMORY_POOL_BITMAP_ADDR + 128 * 1024)

#define KERNEL_HEAP_START_VADDR 0xC0800000


static memory_pool ph_memory_pool;
static memory_pool kernel_vmemory_pool;
static struct mem_block_desc g_kernel_block_desc[BLOCK_DESC_SIZE];



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
    lock(&p_memory_pool->lock);
    size_t index = bitmap_find_range_from_index(&p_memory_pool->bmap, page_count, start_index);
    if(index == BITMAP_RANGE_NOTFOUND){
        unlock(&p_memory_pool->lock);
        return NULL;
    }
    bitmap_range_set(&p_memory_pool->bmap, index, page_count, BIT_STATE_USE);
    p_memory_pool->used += PAGE_SIZE * page_count;
    unlock(&p_memory_pool->lock);
    return (addr_t)(PAGE_SIZE * index + p_memory_pool->start);  
}


void free_page_from_pool(addr_t addr, size_t page_count,memory_pool* p_memory_pool){    
    lock(&p_memory_pool->lock);

    size_t index = ((uintaddr_t)addr - p_memory_pool->start) / PAGE_SIZE;
    bitmap_range_set(&p_memory_pool->bmap, index, page_count, BIT_STATE_UNUSE); 
    p_memory_pool->used -= PAGE_SIZE * page_count; 

    unlock(&p_memory_pool->lock);
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
    mutex_init(&ph_memory_pool.lock);
    //设置已经分配过的内存 1- 24MB范围的内存设置为已经分配
    bitmap_range_set(&ph_memory_pool.bmap,(0x100000-ph_memory_pool.start)/PAGE_SIZE,0x1700000/PAGE_SIZE, BIT_STATE_USE);
    ph_memory_pool.used = 0x1700000;
}

void memory_init(){
    pmemory_pool_init();
    kernel_vmemory_pool_init();
    mem_block_desc_array_init(&g_kernel_block_desc);
}

void kernel_vmemory_pool_init(){
    //todo:: 随内存布局的改变要改变
    kernel_vmemory_pool.start = 0xC0000000;//内核起点
    kernel_vmemory_pool.length = 1024*1024*1024;//长度是1GB
    kernel_vmemory_pool.bmap.len_bit = kernel_vmemory_pool.length / PAGE_SIZE;
    kernel_vmemory_pool.bmap.bits = (uint8_t *)KERNEL_VMEMORY_POOL_BITMAP_ADDR;
    bitmap_init(&kernel_vmemory_pool.bmap);   
    mutex_init(&kernel_vmemory_pool.lock);
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


vaddr_t malloc_page_core(vaddr_t start_vaddr, size_t page_count, memory_pool* p_vmemory_pool, vaddr_t page_dir,uint32_t page_attr){
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
    
    //计算可能有多少个不同的页表, 采用先统一分配的方式，方便执行到一半的时候内存不足的情况下的内存回收
    size_t table_count = get_page_dir_entry_vaddr(((uintaddr_t)start_vaddr + (page_count-1) * PAGE_SIZE), page_dir) - 
                        get_page_dir_entry_vaddr(start_vaddr, page_dir) + 1;
    
    paddr_t table_paddr = pmalloc_page(table_count);
    if(table_paddr == NULL){
        pfree_page(paddr, page_count);
        free_page_from_pool(vaddr, page_count, p_vmemory_pool);
        put_str("pmalloc table faild!\n");
        return NULL;
    }
    //加锁进行保护
    lock(&p_vmemory_pool->lock);
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
            set_page_dir_entry(t_vaddr, (paddr_t)((uintaddr_t)table_paddr + PAGE_SIZE * table_paddr_index), page_dir,
             PAGE_P_ATTR_EXIST | PAGE_RW_ATTR_RW | PAGE_US_ATTR_USER);//系统的页表是固定的，这里能运行到的话只能是用户的，这里可以固定这些属性
            table_paddr_index++;
        }
        set_page_table_entry(t_vaddr, t_paddr, page_dir, page_attr);
    }
    unlock(&p_vmemory_pool->lock);
    // 回收没用到的页表内存
    if(table_paddr_index < table_count){
        pfree_page((paddr_t)((uintaddr_t)table_paddr + PAGE_SIZE * table_paddr_index), 
        table_count - table_paddr_index);
    }
    return vaddr;
}


void free_page_core(vaddr_t vaddr, size_t page_count,  memory_pool* p_vmemory_pool, vaddr_t page_dir){
    lock(&p_vmemory_pool->lock);
    for(size_t i = 0; i < page_count; i++){
        vaddr_t t_vaddr = (vaddr_t)((uintaddr_t)vaddr + i * PAGE_SIZE);
        page* p_page_table_entry = get_page_table_entry_vaddr(t_vaddr, page_dir);
        pfree_page((paddr_t)(p_page_table_entry->PADDR * PAGE_SIZE),1); 
        set_page_table_entry(t_vaddr, NULL, page_dir, PAGE_P_ATTR_UNEXIST);
        //刷新TLB缓存
        asm volatile("invlpg %0"::"m"(t_vaddr):"memory");
    }
    free_page_from_pool(vaddr, page_count, p_vmemory_pool); 
    unlock(&p_vmemory_pool->lock);
}


vaddr_t malloc_kernel_page(size_t page_count){
    return malloc_page_core((vaddr_t)KERNEL_HEAP_START_VADDR, page_count, 
    &kernel_vmemory_pool, (vaddr_t)KERNEL_PAGE_DIR_VADDR, 
    PAGE_P_ATTR_EXIST | PAGE_RW_ATTR_RW | PAGE_US_ATTR_SYS);
}


void free_kernel_page(vaddr_t vaddr, size_t page_count){
     free_page_core(vaddr, page_count, &kernel_vmemory_pool, (vaddr_t)KERNEL_PAGE_DIR_VADDR);
}


vaddr_t malloc_user_page(size_t page_count){
    struct task_struct *pcb = get_current_pcb();
    if(is_kernel_thread(pcb)){
        return NULL;
    }
    return malloc_page_core(USER_VADDR_START,page_count,&pcb->vmemory_pool,
        (vaddr_t)PAGE_DIR_VADDR, PAGE_P_ATTR_EXIST | PAGE_RW_ATTR_RW | PAGE_US_ATTR_USER);
}

void free_user_page(vaddr_t vaddr, size_t page_count){
    struct task_struct *pcb = get_current_pcb();
    if(is_kernel_thread(pcb)){
        return;
    }
    free_page_core(vaddr, page_count,&pcb->vmemory_pool, (vaddr_t)PAGE_DIR_VADDR);
}

vaddr_t malloc_page(size_t page_count){
    struct task_struct *pcb = get_current_pcb();
    if(is_kernel_thread(pcb)){
        return malloc_kernel_page(page_count);
    }
    else{
        return malloc_user_page(page_count);
    }
}

void free_page(vaddr_t vaddr, size_t page_count){
    struct task_struct *pcb = get_current_pcb();
    if(is_kernel_thread(pcb)){
        return free_kernel_page(vaddr,page_count);
    }
    else{
        return free_user_page(vaddr,page_count);
    }
}

void user_vmemory_pool_init(memory_pool* p_user_vmemory_pool){
    size_t page_count = (USER_PROCESS_MEMORY_MAX_LENGTH + PAGE_SIZE - 1)/ PAGE_SIZE;
    size_t bitmap_size_byte = (page_count + 8 - 1) / 8;
    size_t bitmap_size_page = (bitmap_size_byte + PAGE_SIZE - 1) / PAGE_SIZE;
    size_t bitmap_length_bit = page_count;
    p_user_vmemory_pool->start = USER_VADDR_START;
    p_user_vmemory_pool->length = USER_PROCESS_MEMORY_MAX_LENGTH;
    p_user_vmemory_pool->used = 0;
    p_user_vmemory_pool->bmap.len_bit = bitmap_length_bit;
    p_user_vmemory_pool->bmap.bits = malloc_kernel_page(bitmap_size_page);
    bitmap_init(&p_user_vmemory_pool->bmap);
    mutex_init(&p_user_vmemory_pool->lock);
}


void mem_block_desc_array_init(struct mem_block_desc* desc_array){
    size_t block_size = BLOCK_MIN_SIZE;
    for(int i = 0; i < BLOCK_DESC_SIZE;i++){
        desc_array[i].block_size = block_size;
        desc_array[i].blocks_per_arena = (PAGE_SIZE - sizeof(struct arena))/block_size;
        list_init(&desc_array[i].free_list);
        block_size *= 2;
    }
}

struct mem_block* arena2block(struct arena* p_arena, size_t index){
    if(p_arena->large_flag){
        return (struct mem_block*)((uintaddr_t)p_arena + sizeof(struct arena));
    }
    return (struct mem_block*)((uintaddr_t)p_arena + sizeof(struct arena) + p_arena->p_block_desc->block_size*index);
}


struct arena* block2arena(struct mem_block* p_block){
    return (struct arena*)(PAGE_INDEX(p_block) * PAGE_SIZE);
}

struct arena* malloc_and_init_page_arena(size_t page_count){
    struct arena* ret;
    ret = malloc_page(page_count);
    if(NULL == ret){
        return NULL;
    }
    ret->large_flag = true;
    ret->p_block_desc = NULL;
    ret->count.page_count = page_count;
    return ret;
}


struct arena* malloc_and_init_block_arena(struct mem_block_desc* p_block_desc){
    struct arena* ret;
    ret = malloc_page(1);
    if(NULL == ret){
        return NULL;
    }
    ret->large_flag = false;
    ret->p_block_desc = p_block_desc;
    ret->count.free_count = p_block_desc->blocks_per_arena;
    for(int i = 0; i < p_block_desc->blocks_per_arena; i++){
        struct mem_block* p_block = arena2block(ret, i);
        list_push_back(&p_block_desc->free_list, &p_block->free_node);
    }
    return ret;
}


void *sys_malloc(size_t size){
    void *ret = NULL;
    if(size > BLOCK_MAX_SIZE){
        size_t page_count = DIV_ROUND_UP(size + sizeof(struct arena), PAGE_SIZE);
        ret = malloc_and_init_page_arena(page_count);
        ret = arena2block(ret, 0);
    }
    else{
        struct task_struct *pcb = get_current_pcb();
        memory_pool * p_vmemory_pool = NULL;
        struct mem_block_desc* desc_arr = NULL;
        if(is_kernel_thread(pcb)){
            desc_arr = g_kernel_block_desc;
            p_vmemory_pool = &kernel_vmemory_pool;
        }
        else{
            desc_arr = pcb->u_block_desc;
            p_vmemory_pool = &pcb->vmemory_pool;
        }
        //查找是哪个大小的block
        int desc_index = 0;
        for(desc_index = 0; desc_index < BLOCK_DESC_SIZE; desc_index++) {
            if(size <= desc_arr[desc_index].block_size){
                break;
            }
        }
        struct mem_block_desc* p_desc = &desc_arr[desc_index];
        //此类block_size大小的block没了就开始分配一个arena
        lock(&p_vmemory_pool->lock);
        if(list_empty(&p_desc->free_list)){
            malloc_and_init_block_arena(p_desc);
        }
        struct mem_block* p_block = elem2entry(struct mem_block, free_node, list_pop_front(&p_desc->free_list));
        struct arena* p_arena = block2arena(p_block);
        p_arena->count.free_count--;
        unlock(&p_vmemory_pool->lock);
        ret = p_block;
    }
    return ret;
}


void sys_free(void* p){
    if(!p) {
        return;
    }
    struct arena* p_arena = block2arena(p);
    if(p_arena->large_flag){
        free_page(p_arena, p_arena->count.page_count);
    }
    else{
        struct task_struct *pcb = get_current_pcb();
        memory_pool * p_vmemory_pool = NULL;
        if(is_kernel_thread(pcb)){
            p_vmemory_pool = &kernel_vmemory_pool;
        }
        else{
            p_vmemory_pool = &pcb->vmemory_pool;
        }
        lock(&p_vmemory_pool->lock);
        struct mem_block_desc* p_block_desc = p_arena->p_block_desc;
        list_push_front(&p_block_desc->free_list, p);
        p_arena->count.free_count++;
        if(p_arena->count.free_count == p_block_desc->blocks_per_arena){
            //空闲的arena释放掉
            for(int i = 0; i < p_block_desc->blocks_per_arena; i++){
                struct mem_block* p_block = arena2block(p_arena, i);
                list_remove(&p_block->free_node);
            }
            free_page(p_arena, 1);
        }
        unlock(&p_vmemory_pool->lock);
    }
}