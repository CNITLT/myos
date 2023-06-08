#include "print.h"
#include "io.h"
#include "init.h"
#define __str(x) #x
#define str(x) __str(x)
#define TestInt(num) case num:asm(str(int $##num));break;
void timer_interrupt(void){
    static int count = 0;
    count++;
    put_str("this is timer interrupt func count:");
    put_int(count);
    put_char('\n');
}
int main(){
    clear_screen();
    set_cursor_loc(0);
    init_all();
    open_interrupt();
    register_interrupt_func(0x20, timer_interrupt);
    while(1){
       
        
    }
    return 0;
}
