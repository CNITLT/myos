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
#include "tss.h"
#include "process.h"
#include "syscall.h"
#define __str(x) #x
#define str(x) __str(x)
#define TestInt(num) case num:asm(str(int $##num));break;


void thread1(void* args){
    //open_interrupt();
    sync_printf("thread1:%x interupt_state:%d\n", thread1, get_interrupt_state()); 
    while(1){
        //sync_printf("thread1:%x read:%c\n", thread1, read_ascii_from_keyboard_ioqueue()); 
        //thread_yield();//这个让渡还是得放开，不然就一个线程先全部读完，调度，第二个线程还是没得读
        printf("thread1\n");
        for(int i = 0;i<1024;i++){
            for(int j = 0; j < 1024;j++){}
        }
    }
}

void thread2(void* args){
    //open_interrupt();
    sync_printf("thread2:%x interupt_state:%d\n", thread2, get_interrupt_state()); 
    while(1){ 
        printf("thread2\n");
        for(int i = 0;i<1024;i++){
            for(int j = 0; j < 1024;j++){}
        }
        //sync_printf("thread2:%x read:%c\n", thread2, read_ascii_from_keyboard_ioqueue()); 
        //thread_yield();
    }
}


void process1(){
    while(1){
        //不能用sync_printf里面的锁是内核的东西，用户态访问不到
        //thread_yield也不行，目前很多函数都是直接用了内核的东西，用户态一用就出问题
        //printf是因为显存的地址给用户态了,能直接改显存
        //目前的代码内核态有些地方是用户态能访问的，权限给的有些乱
        printf("process1\n");
        for(int i = 0;i<1024;i++){
            for(int j = 0; j < 1024;j++){}
        }
        
        //thread_yield();//这个让渡还是得放开，不然就一个线程先全部读完，调度，第二个线程还是没得读
    }
}

void process2(){
    while(1){
        printf("process2\n"); 
        for(int i = 0;i<1024;i++){
            for(int j = 0; j < 1024;j++){}
        }
        //thread_yield();//这个让渡还是得放开，不然就一个线程先全部读完，调度，第二个线程还是没得读
    }
}

void init_thread(void *args){
    close_interrupt();
    thread_start("thread1",1,thread1,NULL);
    thread_start("thread2",1,thread2,NULL);
    process_execute(process1,"p1");
    process_execute(process2,"p2");
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
    sync_printf("gdt limit:0x%x\n", get_gdt_limit());
    sync_printf("sizeof(tss):%d\n", sizeof(struct tss));
    pf_gdt_entry((struct gdt_entry *)get_gdt_addr()+1);
    
    struct gdt_ptr old_gdt_ptr = get_gdt_ptr();
    vaddr_t new_gdt = malloc_kernel_page(1);
    memcpy(new_gdt,gdt, 64 * 8);
    struct gdt_ptr gdt_ptr;
    gdt_ptr.base = new_gdt;
    gdt_ptr.limit = old_gdt_ptr.limit;
    set_gdt(&gdt_ptr);
    /*
    int p_count = 0;
    for(int i = 0; i < 1024;i++){
        page* page_dir_entry = (page *)((uintaddr_t)(PAGE_DIR_VADDR) + i * 4);
        p_count += page_dir_entry->P;
        printf("i:%d p: %d p_count:%d\n",i, page_dir_entry->P, p_count);
    }
    */
   
    printf("start_process:%x\n",start_process);

    printf("cr3:0X%x\n",get_cr3_register()); 
    syscall(0);
    //open_interrupt();
    init_thread_boot(init_thread, NULL);

   
    while(1);
    return 0;
}

