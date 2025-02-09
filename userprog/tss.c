#include "tss.h"
#include "gdt.h"
#include "page.h"

void init_tss(void){
    //设置g_tss
    g_tss.ss0 = GDT_SELECTOR_DATA;
    g_tss.io_base = sizeof(struct tss);//超过GDT里的界限值则表明没有IO位图,下面的界限值是sizeof(struct tss)-1
    
    struct gdt_entry* p_gdt = (struct gdt_entry*)get_gdt_addr();
    struct gdt_entry* p_gdt_tss = p_gdt + 3;//设置第4个为TSS表项值
    init_gdt_entry_with_default_config_and_param(p_gdt_tss, GDT_ENTRY_S_SYSTEM, GDT_ENTRY_TYPE_TSS, GDT_ENTRY_DPL_0);
    //先置0，其实没设置好
    p_gdt_tss->p = 0;
    //TSS这db和l两个必须置0
    p_gdt_tss->db = 0;
    p_gdt_tss->l = 0;
    //需要重设TSS的段基址和段界限两个值
    set_gdt_entry_seg_base_addr(p_gdt_tss,&g_tss);
    set_gdt_entry_seg_limit(p_gdt_tss, sizeof(struct tss)-1);
    //设置好了再重新置为1
    p_gdt_tss->p = 1;

    //上面设置好了GDT表项数据，但GDT界限值还没更新到
    //更新界限值
    struct gdt_ptr new_gdt_ptr = get_gdt_ptr();    
    new_gdt_ptr.limit += sizeof(struct gdt_entry);
    set_gdt(&new_gdt_ptr);
    //设置tr寄存器，该寄存器保存GDT指向TSS的选择子
    set_tr_register(GDT_SELECTOR_GLOBAL_TSS);
}

void update_tss_esp0(struct task_struct* pcb){
    g_tss.esp0 = (uint32_t)((uint32_t)pcb + PAGE_SIZE);
}

void set_tr_register(uint16_t tss_selector){
    asm volatile (
        "ltr %w0"::"r"(tss_selector)
    );
}