#include "process.h"
#include "thread.h"
#include "string.h"
#include "gdt.h"
#include "interrupt.h"
#include "eflags.h"
#include "memory.h"
#include "print.h"
#include "page.h"
#include "list.h"
#include "debug.h"
#include "tss.h"
void user_gdt_init(){
    struct gdt_entry* p_gdt = (struct gdt_entry*)get_gdt_addr();
    struct gdt_entry* p_gdt_user_code = p_gdt + (GDT_SELECTOR_USER_CODE >> 3);//设置第4个为用户代码段
    struct gdt_entry* p_gdt_user_data = p_gdt + (GDT_SELECTOR_USER_DATA >> 3);//设置第5个为用户数据段
    
    init_gdt_entry_with_default_config_and_param(p_gdt_user_code, GDT_ENTRY_S_DATA, GDT_ENTRY_TYPE_CODE, GDT_ENTRY_DPL_3);
    init_gdt_entry_with_default_config_and_param(p_gdt_user_data, GDT_ENTRY_S_DATA, GDT_ENTRY_TYPE_DATA_W, GDT_ENTRY_DPL_3);     

    //上面设置好了GDT表项数据，但GDT界限值还没更新到
    //更新界限值
    struct gdt_ptr new_gdt_ptr = get_gdt_ptr();    
    new_gdt_ptr.limit += 2 * sizeof(struct gdt_entry);
    set_gdt(&new_gdt_ptr);
}

void start_process(void* filename){

    void *function = filename;
    struct task_struct* cur_pcb = get_current_pcb();
    //切换到中断栈进行操作，毕竟是进程要通过中断来跳转到用户态
    cur_pcb->self_kernel_stack = (uint32_t)cur_pcb->self_kernel_stack + sizeof(struct thread_stack);
    struct interrupt_stack* p_intr_stack =  (struct interrupt_stack*)cur_pcb->self_kernel_stack;
    memset(p_intr_stack, 0, sizeof(struct interrupt_stack));
    p_intr_stack->ds = GDT_SELECTOR_USER_DATA;
    p_intr_stack->es = GDT_SELECTOR_USER_DATA;
    p_intr_stack->fs = GDT_SELECTOR_USER_DATA;
    p_intr_stack->cs = GDT_SELECTOR_USER_CODE;
    p_intr_stack->gs = GDT_SELECTOR_USER_DATA;
    p_intr_stack->eip = function;
    p_intr_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_IF_1 | EFLAGS_MBS);
 
    p_intr_stack->esp = (uintaddr_t)malloc_page_core(USER_STACK3_VADDR,1,&cur_pcb->vmemory_pool,
          PAGE_DIR_VADDR, PAGE_P_ATTR_EXIST | PAGE_RW_ATTR_RW | PAGE_US_ATTR_USER) 
         + PAGE_SIZE - 16;//16是当缓冲区用的
 
    p_intr_stack->ss = GDT_SELECTOR_USER_DATA;
    //debug("start_process end\n");
 
    intr_exit_from(p_intr_stack);
}

vaddr_t create_page_dir(void){
  
   vaddr_t page_dir_vaddr = malloc_kernel_page(1);
   memset(page_dir_vaddr,0,PAGE_SIZE);
   if (page_dir_vaddr == NULL) {
      printf("create_page_dir: malloc_kernel_page(1) failed!\n");
      return NULL;
   }

   // 复制高1GB的内核页目录项目
   memcpy((vaddr_t)((uintaddr_t)page_dir_vaddr + 1024*3), (vaddr_t)((uintaddr_t)PAGE_DIR_VADDR + 1024*3),1024);
   //第一项也复制一下，前4MB是直接映射，也是归属内核的
   memcpy((vaddr_t)((uintaddr_t)page_dir_vaddr), (vaddr_t)((uintaddr_t)PAGE_DIR_VADDR),4);
   //页目录的物理地址更新，这个每个进程都是独立的，所以不会影响到内核，且内核的二级页目录都是固定的，全部进程共享,修改也会被所有进程看到
   paddr_t page_dir_paddr = vaddr2paddr(page_dir_vaddr, PAGE_DIR_VADDR);
   /* 页目录地址是存入在页目录的最后一项,更新页目录地址为新页目录的物理地址 */
   page* p_dir_entry = ((page*)page_dir_vaddr + 1023);
   p_dir_entry->PADDR = PAGE_INDEX(page_dir_paddr);
   p_dir_entry->US = PAGE_US_VALUE_SYS;
   p_dir_entry->RW = PAGE_RW_VALUE_RW;
   p_dir_entry->P = PAGE_P_VALUE_EXIST;

   return page_dir_vaddr;
}

struct task_struct* process_execute(void* filename, char* name){
   // 先申请一个PCB
   struct task_struct* pcb = malloc_kernel_page(1);
   //初始化PCB信息
   init_pcb(pcb, name, USER_PROCESS_DEFAULT_PRIOR); 
   user_vmemory_pool_init(&pcb->vmemory_pool);
   thread_create(pcb, start_process, filename);

   //页表创建
   pcb->page_dir = create_page_dir();
   
   enum interrupt_state old_state = close_interrupt();

   assert(!find_node(&thread_ready_list, &pcb->general_tag));
   list_push_back(&thread_ready_list, &pcb->general_tag);

   assert(!find_node(&thread_all_list, &pcb->all_list_tag));
   list_push_back(&thread_all_list, &pcb->all_list_tag);

   set_interrupt_state(old_state);
   //printf("process_execute\n");
   return pcb;
}



void process_activate(struct task_struct* pcb){
    /* 击活该进程或线程的页表 */
    //debug("before page_dir_activate\n"); 
    //debug("pcb != &main_thread_pcb && pcb->page_dir : %d\n",pcb != &main_thread_pcb && pcb->page_dir);
    //debug("before cr3:%x\n",get_cr3_register());
    page_dir_activate(pcb);
    //debug("after page_dir_activate\n"); 
    //debug("after cr3:%x\n",get_cr3_register());
    /* 内核线程特权级本身就是0,处理器进入中断时并不会从tss中获取0特权级栈地址,故不需要更新esp0 */
    if (pcb->page_dir) {
       /* 更新该进程的esp0,用于此进程被中断时保留上下文 */
       //debug("update_tss_esp0\n");
       update_tss_esp0(pcb);
    }
}


void page_dir_activate(struct task_struct* pcb){
    //默认激活内核，如果是正常的用户线程就用用户页表替换掉内核的页表
    paddr_t page_dir_paddr = KERNEL_PAGE_DIR_PADDR;
    if(pcb->page_dir){
        page_dir_paddr = vaddr2paddr(pcb->page_dir, PAGE_DIR_VADDR);
    }
    //debug("page_dir_paddr:%x\n",page_dir_paddr);
    set_cr3_register(page_dir_paddr);
    //debug("page_dir_paddr:%x\n",page_dir_paddr);
}