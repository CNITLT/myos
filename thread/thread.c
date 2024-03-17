#include "thread.h"
#include "page.h"
#include "string.h"
#include "interrupt.h"
#include "memory.h"
#include "debug.h"
#include "io.h"
#define MIN_TICKS 1
struct task_struct* main_thread_pcb; //主线程PCB，等会启动的时候切换到这个线程，保证模型一致
struct list thread_ready_list; //就绪队列
struct list thread_all_list;//总队列


/* 由kernel_thread去执行function(func_arg) */
static void kernel_thread(thread_func* function, void* func_arg) {
/* 执行function前要开中断,避免后面的时钟中断被屏蔽,而无法调度其它线程 */
   open_interrupt();
   //发送中断结束命令EOI
   outb(0xa0, 0x20);
   outb(0x20, 0x20);
   function(func_arg); 
}

NAKEDFUNC static void switch_to(struct task_struct* cur, struct task_struct* next){
    asm volatile(" \
    push %esi; \
    push %edi; \
    push %ebx; \
    push %ebp; \
    movl 20(%esp), %eax; \
    movl %esp, (%eax); \
    ");//cur = [esp+20]

   asm volatile(" \
    movl 24(%esp), %eax; \
    movl (%eax), %esp; \
    pop %ebp; \
    pop %ebx; \
    pop %edi; \
    pop %esi; \
    ret; \
    "); 
}


void thread_create(struct task_struct* pcb, thread_func function, void* func_arg){
    pcb->self_kernel_stack = (uint32_t)pcb->self_kernel_stack - sizeof(struct interrupt_stack) - sizeof(struct thread_stack);
    struct thread_stack* stack = pcb->self_kernel_stack;
    stack->eip = kernel_thread;
    stack->function = function;
    stack->func_arg = func_arg;
    stack->ebp = 0;
    stack->ebx = 0;
    stack->esi = 0;
    stack->edi = 0;
    
}
void init_pcb(struct task_struct* pcb, char* name, int priority){
    memset(pcb, 0, sizeof(struct task_struct));
    memcpy(pcb->name, name, strlen(name));
    pcb->status = TASK_RUNNING;
    pcb->priority = priority > MIN_TICKS?priority:MIN_TICKS;
    pcb->ticks = pcb->priority;
    pcb->elapsed_ticks = 0;
    pcb->page_dir = NULL;
    if(pcb == main_thread_pcb){
        pcb->status = TASK_RUNNING;
    }
    else{
        pcb->status = TASK_READY;
    }
    
    pcb->self_kernel_stack = (vaddr_t)((uint32_t)pcb + PAGE_SIZE);
    pcb->stack_magic = STACK_OVERFLOW_MAGIC_NUM;
}


struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg){
    struct task_struct* pcb = malloc_kernel_page(1);
    init_pcb(pcb, name, prio);
    thread_create(pcb, function, func_arg);
    list_push_back(&thread_ready_list, &pcb->general_tag);
    list_push_back(&thread_all_list, &pcb->all_list_tag);
    return pcb;
}


struct task_struct* get_current_pcb(void){
    vaddr_t esp;
    asm volatile("movl %%esp, %0":"=g"(esp));
    return (uint32_t)esp & 0xFFFFF000;
}


void init_thread_boot(thread_func main_function, void* func_arg){
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    main_thread_pcb = malloc_kernel_page(1);
    init_pcb(main_thread_pcb, "main", 10);
    thread_create(main_thread_pcb, main_function, func_arg);
    list_push_back(&thread_all_list, &main_thread_pcb->all_list_tag);

    asm volatile(" \
    movl %0, %%esp; \
    pop %%ebp; \
    pop %%ebx; \
    pop %%edi; \
    pop %%esi; \
    ret; \
    "::"m"(main_thread_pcb->self_kernel_stack):"memory");
}




/* 实现任务调度 */
void schedule() {
    assert(get_interrupt_state() == INTERRUPT_DISABLE);
   struct task_struct* cur = get_current_pcb(); 
   if (cur->status == TASK_RUNNING) { // 若此线程只是cpu时间片到了,将其加入到就绪队列尾
        cur->ticks = cur->priority;     // 重新将当前线程的ticks再重置为其priority;
        cur->status = TASK_READY;
        list_push_back(&thread_ready_list, &cur->general_tag);
   } else { 
      /* 若此线程需要某事件发生后才能继续上cpu运行,
      不需要将其加入队列,因为当前线程不在就绪队列中。*/
   }
   struct list_node* thread_tag = list_pop_front(&thread_ready_list);   
   struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);

    
   // clear_screen();
    put_str("debug:\n");
    put_hex(cur);
    put_str("\n"); 
    put_hex(next);
    put_str("\n");
for(int i = 0;i<1024*1024;i++){
    for(int j = 0; j < 100;j++){

    }
}

    static int count = 0;
    if(!count){
        put_str("count addr:");
        put_hex(&count);
        put_str("\n"); 
    }
    count++;
   next->status = TASK_RUNNING;
   switch_to(cur, next);
}