#include "keyboard.h"
#include "print.h"
#define KEYBOARD_INTERUPT_NUM 0x21
#define KEYBOARD_BUFF_PORT 0x60
void keyboard_interupt(void){
    sync_printf("keyboard_interupt data:%x\n",  inb(KEYBOARD_BUFF_PORT));
}


void keyboard_init(){
    register_interrupt_func(KEYBOARD_INTERUPT_NUM, keyboard_interupt);
}