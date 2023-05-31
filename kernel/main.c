#include "print.h"
#include "io.h"
#include "interrupt.h"


int main(){
    clear_screen();
    set_cursor_loc(0);
    interrupt_init();
    while(1){
         asm volatile("int $1");
    }
    return 0;
}
