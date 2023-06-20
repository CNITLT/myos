#include "page.h"
#include "debug.h"
//内核页目录和页表的物理地址
#define KERNEL_PAGE_DIR_PADDR 0x400000 
//内核页目录和页表的虚拟地址
#define KERNEL_PAGE_DIR_VADDR 0xFFFFF000

#define PAGE_DIR_INDEX(ADDR) (((size_t)ADDR>>22)&0x3FF)
#define PAGE_TABLE_INDEX(ADDR) (((size_t)ADDR>>12)&0x3FF)




page_vaddr_t get_page_table_entry_vaddr(vaddr_t addr, vaddr_t page_dir){
    assert(((size_t)page_dir & 0xFFF) == 0); //页目录地址是4KB对齐
    assert(PAGE_DIR_INDEX(page_dir) == PAGE_TABLE_INDEX(page_dir));//页目录和页表汇集在一起，且页目录指向自己的时候，地址一定会满足这个条件.
    page_vaddr_t entry_addr = (page_vaddr_t)(
        ((size_t)page_dir & 0xFFC00000) + 
        (PAGE_DIR_INDEX(addr) * (PAGE_SIZE / sizeof(page))) + 
        (PAGE_DIR_INDEX(addr) * sizeof(page))
        );
    return entry_addr;
}

page_vaddr_t get_page_dir_entry_vaddr(vaddr_t addr, vaddr_t page_dir){
    assert(((size_t)page_dir & 0xFFF) == 0); //页目录地址是4KB对齐
    assert(PAGE_DIR_INDEX(page_dir) == PAGE_TABLE_INDEX(page_dir));//页目录和页表汇集在一起，且页目录指向自己的时候，地址一定会满足这个条件.
    page_vaddr_t entry_addr = (page_vaddr_t)((size_t)page_dir + PAGE_DIR_INDEX(addr) * sizeof(page));
    return entry_addr;
}