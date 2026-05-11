#ifndef __KEY_BOARD_H
#define __KEY_BOARD_H
#include "interrupt.h"
#include "stdint.h"
#include "stddef.h"
#include "io.h"
#include "queue.h"

// 这里定义组合键的值, ascii规定如此
#define KEY_CTRL(char) (char & 0x1f)

/*
@brief 键盘中断函数
*/
void keyboard_interupt(void);


/*
@brief 键盘中断初始化
*/
void keyboard_init();

/*
@brief 从IO队列里面读取ascii码字符
*/
char read_ascii_from_keyboard_ioqueue();



#endif