#include "init.h"
void init_all(){
    interrupt_init();
    timer_init();
    memory_pool_init();
}