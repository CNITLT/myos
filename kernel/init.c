#include "init.h"
#include "timer.h"
#include "interrupt.h"
#include "memory.h"
#include "stddef.h"
#include "stdint.h"
#include "print.h"
#include "keyboard.h"
void init_all(){
    interrupt_state old_state = close_interrupt();
    interrupt_init();
    timer_init();
    memory_pool_init();
    console_init();
    keyboard_init();
    set_interrupt_state(old_state);
}

