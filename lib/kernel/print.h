#ifndef __PRINT_H
#define __PRINT_H
#include "stdint.h"
#define VGA_TXT_MODE_START_ADDR  0xB8000
#define BLACK_BACKGROUND_WHITE_CHAR 0x7 //黑底白字
#define VGA_END_ADDR  0xBFFFF //显存的末尾地址
#define VGA_CRT_ADDR_REG_PORT  0x3D4 //CRT地址寄存器的端口地址，默认是这个，可能会被其它设置影响
#define VGA_CRT_DATA_REG_PORT  0x3D5 //CRT数据寄存器的端口地址，默认是这个，可能会被其它设置影响
#define VGA_CRT_CURSOR_LOW  0x0F //CRT里光标低8位寄存器
#define VGA_CRT_CURSOR_HIGH  0x0E //CRT里光标高8位寄存器
void put_char(char ch);
void put_str(char* str);
void put_int(int32_t num);
void clear_screen();
uint32_t strlen(char* str);
uint32_t read_cursor_loc();
void set_cursor_loc(uint32_t pos);
void roll_up();

#endif