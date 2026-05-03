#include "init.h"
#include "timer.h"
#include "interrupt.h"
#include "memory.h"
#include "stddef.h"
#include "stdint.h"
#include "print.h"
#include "keyboard.h"
#include "tss.h"
#include "process.h"
#include "syscall_init.h"
#include "ide.h"
#include "fs.h"
#include "dir.h"
#include "inode.h"
void init_all(){
    interrupt_state old_state = close_interrupt();
    
    interrupt_init();
    timer_init();
    memory_init();
    console_init();
    keyboard_init();
    init_tss();
    user_gdt_init();
    syscall_init();
    //ide_init(); 这个放这里不太行
    set_interrupt_state(old_state);
}

void init_other_in_main_thread(){
    ide_init(); 
    fileSystem_init();
    load_default_partition();
    open_root_dir(g_current_part);
}
