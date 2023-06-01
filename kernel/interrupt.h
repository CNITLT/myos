#ifndef __INTERRUPT_H
#define __INTERRUPT_H
#include "stdint.h"

//实际的中断函数类型
typedef void (*interrupt_func_handler)(void);
/*
@brief 注册中断函数
@param INTERRUPT_NUM:uint16_t 中断向量号
@param func:interrupt_func_handler 中断函数
@return interrupt_func_handler 旧中断函数
*/
interrupt_func_handler register_interrupt_func(uint16_t INTERRUPT_NUM, interrupt_func_handler func);

/*
@brief 中断机制初始化,包含了IDT,中断控制器的初始化
*/
void interrupt_init();

/*
@brief 开中断
*/
void open_interrupt();

/*
@brief 关中断
*/
void close_interrupt();
#endif