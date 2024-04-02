#ifndef __KEY_BOARD_H
#define __KEY_BOARD_H
#include "interrupt.h"
#include "stdint.h"
#include "stddef.h"
#include "io.h"


/*
@brief 键盘中断函数
*/
void keyboard_interupt(void);


/*
@brief 键盘中断初始化
*/
void keyboard_init();


#endif