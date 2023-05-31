#ifndef __INTERRUPT_H
#define __INTERRUPT_H
#include "stdint.h"
//实际的中断函数类型
typedef void (*interrupt_func_handler)(void);
//中断门描述符定义
typedef struct interrupt_gate_desc{
    uint16_t func_offset_low; //中断处理程序在目标代码段的偏移中的低16位
    uint16_t code_selector; //目标代码段选择子
    struct {
        uint16_t fixed:11; //对中断门来说是固定的几位，高地址110 0000 0000低地址 
        uint16_t D:1; //D为1表示32位，为0是16位
        uint16_t S:1; //固定的为0
        uint16_t DPL:2; //特权级
        uint16_t P:1; //存在位
    } attribute;
    uint16_t func_offset_high; //中断处理程序在目标代码段的偏移中的高16位
} interrupt_gate_desc;

#define DEFAULT_INTERRUPT_GATE_DESC_VALUE {0,(1 << 3),{0x600,1,0,0,1},0}
#define IDT_SIZE 32
extern interrupt_gate_desc IDT[];//声明全局的IDT表，实际定义在.c文件中
/*
@brief 初始化idt表内所有描述符
*/
void idt_desc_init(void);

/*
@brief 注册中断函数
@param INTERRUPT_NUM:uint16_t 中断向量号
@param func:interrupt_func_handler 中断函数
@return interrupt_func_handler 旧中断函数
*/
interrupt_func_handler register_interrupt_func(uint16_t INTERRUPT_NUM, interrupt_func_handler func);

/*
@brief 中断机制初始化
*/
void interrupt_init();
#endif