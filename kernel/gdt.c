#include "gdt.h"
#include "print.h"
#include "string.h"

vaddr_t get_gdt_addr(){
    struct gdt_ptr gdt_ptr;
    asm volatile (
        "sgdt %0":"=m"(gdt_ptr)::"memory"
    );
    return gdt_ptr.base;
}

uint16_t get_gdt_limit(){
    struct gdt_ptr gdt_ptr;
    asm volatile (
        "sgdt %0":"=m"(gdt_ptr)::"memory"
    );
    return gdt_ptr.limit; 
}

struct gdt_ptr get_gdt_ptr(){
    struct gdt_ptr gdt_ptr;
    asm volatile (
        "sgdt %0":"=m"(gdt_ptr)::"memory"
    );
    return gdt_ptr;  
}

struct gdt_ptr set_gdt(struct gdt_ptr* p_gdt_ptr){
    struct gdt_ptr ret = get_gdt_ptr();
    
    asm volatile (
        "lgdt (%0)"::"r"(p_gdt_ptr):"memory"
    );
    
    return ret;
}



void set_gdt_entry_seg_base_addr(struct gdt_entry* p_gdt_entry,addr_t addr){
   uint32_t uaddr = (uint32_t)addr;
   p_gdt_entry->seg_base_addr_0_15 = uaddr & 0xFFFF; 
   p_gdt_entry->seg_base_addr_16_23 = (uaddr >> 16) & 0xFF;
   p_gdt_entry->seg_base_addr_24_31 = (uaddr >> 24) & 0xFF;
}

addr_t get_gdt_entry_seg_base_addr(struct gdt_entry* p_gdt_entry){
   uint32_t uaddr = 0;
   uaddr |= p_gdt_entry->seg_base_addr_0_15;
   uaddr |= p_gdt_entry->seg_base_addr_16_23 << 16;
   uaddr |= p_gdt_entry->seg_base_addr_24_31 << 24;
   return (addr_t)uaddr;
}

void set_gdt_entry_seg_limit(struct gdt_entry* p_gdt_entry,uint32_t seg_limit){
    p_gdt_entry->seg_limit_0_15 = seg_limit & 0xFFFF;
    p_gdt_entry->seg_limit_16_19 = (seg_limit >> 16) & 0xF;
}

uint32_t get_gdt_entry_seg_limit(struct gdt_entry* p_gdt_entry){
    uint32_t seg_limit = 0;
    seg_limit |= p_gdt_entry->seg_limit_0_15;
    seg_limit |= p_gdt_entry->seg_limit_16_19 << 16;
    return seg_limit;
}

void init_gdt_entry_part_with_default_config(struct gdt_entry* p_gdt_entry){
    memset(p_gdt_entry,0, sizeof(struct gdt_entry));
    set_gdt_entry_seg_base_addr(p_gdt_entry, 0);
    set_gdt_entry_seg_limit(p_gdt_entry, 0xFFFFF);
    p_gdt_entry->g = 1; //4KB
    p_gdt_entry->db = 1; //32位操作数和栈指针
    p_gdt_entry->l = 0;//非64位
    p_gdt_entry->p = 0;//不存在，因为会有其它的数据没配置,等配置完后手动置1
}


void init_gdt_entry_with_default_config_and_param(struct gdt_entry* p_gdt_entry,uint16_t s, uint16_t type, uint16_t dpl){
    init_gdt_entry_part_with_default_config(p_gdt_entry);
    p_gdt_entry->s = s;
    p_gdt_entry->type = type;
    p_gdt_entry->dpl = dpl;
    p_gdt_entry->p = 1;
}

void pf_gdt_entry(struct gdt_entry* p_gdt_entry){
    sync_printf("seg_base:0X%x\n", get_gdt_entry_seg_base_addr(p_gdt_entry));
    sync_printf("seg_limit:0X%x\n", get_gdt_entry_seg_limit(p_gdt_entry));
    sync_printf("G:%d\n",p_gdt_entry->g);    
    sync_printf("DB:%d\n",p_gdt_entry->db);    
    sync_printf("L:%d\n",p_gdt_entry->l);
    sync_printf("AVL:%d\n",p_gdt_entry->avl);
    sync_printf("P:%d\n",p_gdt_entry->p);
    sync_printf("DPL:%d\n",p_gdt_entry->dpl);
    sync_printf("S:%d\n",p_gdt_entry->s);
    sync_printf("TYPE:%d\n",p_gdt_entry->type);
}