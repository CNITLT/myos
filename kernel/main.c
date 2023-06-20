#include "print.h"
#include "io.h"
#include "init.h"
#include "debug.h"
#include "string.h"
#include "memory.h"
#include "e820.h"
#include "page.h"
#include "stddef.h"
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
    /*
    clear_screen();
    set_cursor_loc(0);
    init_all();
    register_interrupt_func(0x20, timer_interrupt);
   
    //open_interrupt();
    close_interrupt();
*/
    print_e820_table();
    
    pmemeory_pool_init();
    addr_t addr;
    size_t count;
    do{
        addr = pmalloc_page(0x100000/4096);
        put_int(count);
        count++;
        put_str(":");
        put_hex((uint32_t)addr);
        put_str("\n");
    }while(addr);
    put_hex((uint32_t)&addr);
    while(1){ 
    }
    return 0;
}
