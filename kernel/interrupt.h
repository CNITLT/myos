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


typedef 
enum interrupt_state{INTERRUPT_DISABLE = 0,INTERRUPT_ENABLE}
interrupt_state;
/*
@brief 中断机制初始化,包含了IDT,中断控制器的初始化
*/
void interrupt_init();



/*
@brief 读取当前中断状态
@return interrupt_state 当前中断状态
*/
interrupt_state get_interrupt_state();
/*
@brief 开中断
@return interrupt_state 之前的中断状态
*/
interrupt_state open_interrupt();

/*
@brief 关中断
@return interrupt_state 之前的中断状态
*/
interrupt_state close_interrupt();


/*
@brief 设置中断状态
@return interrupt_state 之前的中断状态
*/
interrupt_state set_interrupt_state(interrupt_state state);

#endif