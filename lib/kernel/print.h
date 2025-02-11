#ifndef __LIB_KERNEL_PRINT_H
#define __LIB_KERNEL_PRINT_H
#include "stdint.h"
#define VGA_TXT_MODE_START_ADDR  0xB8000
#define BLACK_BACKGROUND_WHITE_CHAR 0x7 //黑底白字
#define VGA_END_ADDR  0xBFFFF //显存的末尾地址
#define VGA_CRT_ADDR_REG_PORT  0x3D4 //CRT地址寄存器的端口地址，默认是这个，可能会被其它设置影响
#define VGA_CRT_DATA_REG_PORT  0x3D5 //CRT数据寄存器的端口地址，默认是这个，可能会被其它设置影响
#define VGA_CRT_CURSOR_LOW  0x0F //CRT里光标低8位寄存器
#define VGA_CRT_CURSOR_HIGH  0x0E //CRT里光标高8位寄存器



/*
@brief 初始化打印功能,主要是用来初始化锁
*/
void console_init();
/*
@brief 获取显存字符模式下对应的虚拟地址起点
@return vaddr_t 显存虚拟地址起点
*/
vaddr_t get_vga_txt_mode_start_addr();
/*
@brief 在当前光标位置输出一个字符
@param ch:char 输出的字符
*/
void put_char(char ch);

/*
@brief 在当前光标位置输出一个字符串
@param str:char* 字符串的起始地址
*/
void put_str(char* str);

/*
@brief 在当前光标位置以十进制输出数字
@param num:int32_t 输出的数字
*/
void put_int(int32_t num);

/*
@brief 在当前光标位置以十六进制输出数字
@param num:uint32_t 输出的数字
*/
void put_hex(uint32_t num);

/*
@brief 在当前光标位置以十六进制输出数字
@param num:uint64_t 输出的数字
*/
void put_hex64(uint64_t num);
/*
@brief 清空屏幕
*/
void clear_screen();

/*
@brief 原printf的仿制品，仅支持 %d %s %x %c
*/
void printf(const char * p,...);
/*
@brief 读取当前的光标位置
@return uint32_t 光标位置
*/
uint32_t read_cursor_loc();

/*
@brief 设置光标位置
@param pos:uint32_t 光标位置
*/
void set_cursor_loc(uint32_t pos);

/*
@brief 向上滚动一行屏幕
*/
void roll_up();

/*
@brief 对输出加锁的printf，保证输出顺序不乱
*/
void sync_printf(const char * p,...);


#endif