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
#include "mutex.h"
#include "keyboard.h"
#include "gdt.h"
#define __str(x) #x
#define str(x) __str(x)
#define TestInt(num) case num:asm(str(int $##num));break;


void thread1(void* args){
    //open_interrupt();
    sync_printf("thread1:%x interupt_state:%d\n", thread1, get_interrupt_state()); 
    while(1){
        sync_printf("thread1:%x read:%c\n", thread1, read_ascii_from_keyboard_ioqueue()); 
        thread_yield();//这个让渡还是得放开，不然就一个线程先全部读完，调度，第二个线程还是没得读
    }
}

void thread2(void* args){
    //open_interrupt();
    sync_printf("thread2:%x interupt_state:%d\n", thread2, get_interrupt_state()); 
    while(1){ 
        sync_printf("thread2:%x read:%c\n", thread2, read_ascii_from_keyboard_ioqueue()); 
        thread_yield();
    }
}

void init_thread(void *args){
    close_interrupt();
    thread_start("thread1",1,thread1,NULL);
    thread_start("thread2",1,thread2,NULL);
    open_interrupt();
    sync_printf("init_thread:%x interupt_state:%d\n", init_thread, get_interrupt_state()); 
    
    while(1){     
        //sync_printf("init_thread:%x read:%c\n", init_thread, read_ascii_from_keyboard_ioqueue());  
        //thread_yield();
    }
}


int main(){
   
    clear_screen();
    set_cursor_loc(0);
    print_e820_table();
 
    init_all();
    close_interrupt();
    register_interrupt_func(0x20, timer_interrupt); 
    vaddr_t gdt = get_gdt_addr();
    sync_printf("gdt base addr:0x%x\n", gdt);
    vaddr_t new_gdt = malloc_kernel_page(1);
    memcpy(new_gdt,gdt, 64 * 8);
    struct gdt_ptr gdt_ptr;
    gdt_ptr.base = new_gdt;
    gdt_ptr.limit = 3*8 - 1;
    set_gdt(&gdt_ptr);
    //open_interrupt();
    init_thread_boot(init_thread, NULL);

   
    while(1);
    return 0;
}

