#include "page.h"
#include "debug.h"
#include "stddef.h"


page_vaddr_t get_page_table_entry_vaddr(vaddr_t vaddr, vaddr_t page_dir){
    assert(PAGE_MASK(page_dir) == 0); //页目录地址是4KB对齐
    assert(PAGE_DIR_INDEX(page_dir) == PAGE_TABLE_INDEX(page_dir));//页目录和页表汇集在一起，且页目录指向自己的时候，地址一定会满足这个条件.
    page_vaddr_t entry_addr = (page_vaddr_t)(
        ((uintaddr_t)page_dir & 0xFFC00000) + 
        (PAGE_DIR_INDEX(vaddr) * PAGE_SIZE) + 
        (PAGE_TABLE_INDEX(vaddr) * sizeof(page))
        );
    return entry_addr;
}

page_vaddr_t get_page_dir_entry_vaddr(vaddr_t vaddr, vaddr_t page_dir){
    assert(PAGE_MASK(page_dir) == 0); //页目录地址是4KB对齐
    assert(PAGE_DIR_INDEX(page_dir) == PAGE_TABLE_INDEX(page_dir));//页目录和页表汇集在一起，且页目录指向自己的时候，地址一定会满足这个条件.
    page_vaddr_t entry_addr = (page_vaddr_t)((uintaddr_t)page_dir + PAGE_DIR_INDEX(vaddr) * sizeof(page));
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

void set_page_dir_entry(vaddr_t vaddr, paddr_t paddr, vaddr_t page_dir, uint32_t page_attr){
    page* p_page_dir_entry = get_page_dir_entry_vaddr(vaddr, page_dir);
    p_page_dir_entry->PADDR = PAGE_INDEX(paddr);
    uint32_t* p_uint32_page_dir_entry = (uint32_t*)p_page_dir_entry;
    *p_uint32_page_dir_entry &= 0xFFFFF000;
    *p_uint32_page_dir_entry |= (page_attr&0xFFF);
}


error_code_t set_page_table_entry(vaddr_t vaddr, paddr_t paddr, vaddr_t page_dir, uint32_t page_attr){
    page* p_page_dir_entry = get_page_dir_entry_vaddr(vaddr, page_dir);
    if(p_page_dir_entry->P == PAGE_P_VALUE_UNEXIST){
        return ERROR_PAGE_TABLE_UNEXIST;
    }
    page* p_page_table_entry = get_page_table_entry_vaddr(vaddr, page_dir);
    p_page_table_entry->PADDR = PAGE_INDEX(paddr);
    uint32_t* p_uint32_page_table_entry = (uint32_t*)p_page_table_entry; 
    *p_uint32_page_table_entry &= 0xFFFFF000;
    *p_uint32_page_table_entry |= (page_attr&0xFFF); 
    return NOERROR;
}


paddr_t set_cr3_register(paddr_t page_dir_paddr){
    paddr_t old_cr3 = get_cr3_register();
    asm volatile ("movl %0, %%cr3" : : "r" (page_dir_paddr) : "memory");
    return old_cr3;
}


paddr_t get_cr3_register(){
    paddr_t old_cr3 = NULL;
    asm volatile("movl %%cr3, %0":"=g"(old_cr3)::"memory");
    return old_cr3;
}