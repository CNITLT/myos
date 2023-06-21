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
    memory_pool_init();

    addr_t addr;
    while(1){ 
        addr = malloc_page((vaddr_t)KERNEL_HEAP_START_VADDR, 5, &kernel_vmemory_pool, (vaddr_t)KERNEL_PAGE_DIR_VADDR);
        
        put_hex((uint32_t) addr);
        put_str("\n");  
        if(addr == NULL){
            break;
        }
        free_page(addr, 1, &kernel_vmemory_pool, (vaddr_t)KERNEL_PAGE_DIR_VADDR);
    }
    while(1);
    return 0;
}
