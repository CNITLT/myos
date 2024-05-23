#include "gdt.h"


vaddr_t get_gdt_addr(){
    struct gdt_ptr gdt_ptr;
    asm volatile (
        "sgdt %0":"=m"(gdt_ptr)::"memory"
    );
    return gdt_ptr.base;
}


vaddr_t  set_gdt(struct gdt_ptr* p_gdt_ptr){
    vaddr_t ret = get_gdt_addr();
    
    asm volatile (
        "lgdt (%0)"::"r"(p_gdt_ptr):"memory"
    );
    
    return ret;
}