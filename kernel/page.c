#include "page.h"
#include "debug.h"
#include "stddef.h"
//内核页目录和页表的物理地址
#define KERNEL_PAGE_DIR_PADDR 0x400000 
//内核页目录和页表的虚拟地址
#define KERNEL_PAGE_DIR_VADDR 0xFFFFF000

#define PAGE_DIR_INDEX(ADDR) (((size_t)ADDR>>22)&0x3FF)
#define PAGE_TABLE_INDEX(ADDR) (((size_t)ADDR>>12)&0x3FF)

#define PAGE_MASK(ADDR) ((size_t)ADDR & 0xFFF)


page_vaddr_t get_page_table_entry_vaddr(vaddr_t vaddr, vaddr_t page_dir){
    assert(PAGE_MASK(page_dir) == 0); //页目录地址是4KB对齐
    assert(PAGE_DIR_INDEX(page_dir) == PAGE_TABLE_INDEX(page_dir));//页目录和页表汇集在一起，且页目录指向自己的时候，地址一定会满足这个条件.
    page_vaddr_t entry_addr = (page_vaddr_t)(
        ((size_t)page_dir & 0xFFC00000) + 
        (PAGE_DIR_INDEX(vaddr) * PAGE_SIZE) + 
        (PAGE_TABLE_INDEX(vaddr) * sizeof(page))
        );
    return entry_addr;
}

page_vaddr_t get_page_dir_entry_vaddr(vaddr_t vaddr, vaddr_t page_dir){
    assert(PAGE_MASK(page_dir) == 0); //页目录地址是4KB对齐
    assert(PAGE_DIR_INDEX(page_dir) == PAGE_TABLE_INDEX(page_dir));//页目录和页表汇集在一起，且页目录指向自己的时候，地址一定会满足这个条件.
    page_vaddr_t entry_addr = (page_vaddr_t)((size_t)page_dir + PAGE_DIR_INDEX(vaddr) * sizeof(page));
    return entry_addr;
}


paddr_t vaddr2paddr(vaddr_t vaddr, vaddr_t page_dir){
    page* p_page_dir_entry = get_page_dir_entry_vaddr(vaddr, page_dir);
    if(p_page_dir_entry->P == PAGE_P_VALUE_UNEXIST){
        return NULL;
    }
    page* p_page_table_entry = get_page_table_entry_vaddr(vaddr, page_dir);
    if(p_page_table_entry->P == PAGE_P_VALUE_UNEXIST){
        return NULL;
    }
    paddr_t paddr = (p_page_table_entry->PADDR << 12) + PAGE_MASK(vaddr);
    return paddr;
}
