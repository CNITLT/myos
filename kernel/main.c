#include "print.h"
#include "io.h"
#include "init.h"
#include "debug.h"
#include "string.h"
#include "memory.h"
#include "e820.h"
#include "page.h"
#include "stddef.h"
#include "thread.h"
#define __str(x) #x
#define str(x) __str(x)
#define TestInt(num) case num:asm(str(int $##num));break;
void thread2(void *args){
    
    put_str("thread2 eip:");
    put_hex(thread2);
    put_str("\n");
    while(1){
        put_str("thread2\n");
        put_hex((size_t)get_current_pcb());
        put_str("\n");
        put_int(get_interrupt_state());
        put_str("\n");

        for(int i = 0;i<1024*512;i++){
           
        }
    }
}
void thread3(void *args){
    put_str("thread3 eip:");
    put_hex(thread3);
    put_str("\n");
    while(1){
        put_str("thread3\n");
        put_hex((size_t)get_current_pcb());
        put_str("\n");
        put_int(get_interrupt_state());
        put_str("\n");

        for(int i = 0;i<1024*512;i++){
            
        }
    }
}

void init_thread(void *args){

    close_interrupt();
    thread_start("thread2",10,thread2,NULL);
    thread_start("thread3",10,thread3,NULL);
    open_interrupt();

    put_str("init_thread eip:");
    put_hex(init_thread);
    put_str("\n");

    while(1){
        put_str("init thread\n");
        put_hex((size_t)get_current_pcb());
        put_str("\n");
        //put_str("init thread\n");
        
        for(int i = 0;i<1024*512;i++){
            
        }
        
        
    }
}



int main(){
   
    clear_screen();
    set_cursor_loc(0);
    print_e820_table();

    init_all();
    close_interrupt();
    register_interrupt_func(0x20, timer_interrupt); 
    //open_interrupt();
    
    init_thread_boot(init_thread, NULL);

   
    while(1);
    return 0;
}

