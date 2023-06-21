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
    
    clear_screen();
    set_cursor_loc(0);
    print_e820_table();

    init_all();
    register_interrupt_func(0x20, timer_interrupt);
    memory_pool_init();
    //open_interrupt();
    close_interrupt();

   

    addr_t addr;
    while(1){ 
        addr = malloc_kernel_page(1);
        
        put_hex((uint32_t) addr);
        put_str("\n");  
        if(addr == NULL){
            break;
        }
        free_kernel_page(addr,1);
    }
    while(1);
    return 0;
}
