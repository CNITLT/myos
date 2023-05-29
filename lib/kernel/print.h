#ifndef _PRINT_H
#define _PRINT_H
#include "stdint.h"
#define VGA_TXT_MODE_START_ADDR  0xB8000
#define BLACK_BACKGROUND_WHITE_CHAR 0x7 //黑底白字
void put_char(char ch);
void put_str(char* str);
void put_int(int32_t num);
uint32_t strlen(char* str);
extern uint32_t read_cursor_loc();
extern void set_cursor_loc(uint32_t pos);
extern void roll_up();
#endif