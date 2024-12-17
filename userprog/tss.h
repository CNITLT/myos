#ifndef __TSS_H
#define __TSS_H
#include "thread.h"
/*
用于复制的行模板,一行是4个字节，如果只平分两个的话，一边是2个字节
|   xxxxxxxx    |   xxxxxxxx    |


---------------------------------
31                  15            0  
|   IO位图偏移地址   | 保留   |T| 100
|     保留      |   ldt选择子   | 96
|     保留      |      gs       | 92
|     保留      |      fs       | 88
|     保留      |      ds       | 84
|     保留      |      ss       | 80
|     保留      |      cs       | 76
|     保留      |      es       | 72
|              edi              | 68
|              esi              | 64
|              ebp              | 60
|              esp              | 56
|              ebx              | 52
|              edx              | 48
|              ecx              | 44
|              eax              | 40
|             eflags            | 36
|              eip              | 32
|            cr3(pdbr)          | 28
|     保留      |     SS2       | 24
|              esp2             | 20
|     保留      |     SS1       | 16
|              esp1             | 12
|     保留      |     SS0       | 8
|              esp0             | 4
|     保留      |上个任务的TSS指针| 0
----------------------------------
*/
//与原书有改动，全定义成uint32_t
struct tss {
    uint32_t backlink;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1; 
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint32_t trace:1;
    uint32_t __unuse:15;
    uint32_t io_base:16;//IO位图,超过GDT里的界限值则表面没有IO位图
};

static struct tss g_tss;

/*
@brief 初始化全局的TSS相关配置，并添加到GDT中
*/
void init_tss(void);

/*
@brief 更新TSS中的0级栈为当前任务的栈
@param pcb: struct task_struct* :pcb指针
*/
void update_tss_esp0(struct task_struct* pcb);


/*
@brief 设置tr寄存器
@param tss_selector: uint16_t :gdt里TSS描述符的选择子
*/
void set_tr_register(uint16_t tss_selector);
#endif