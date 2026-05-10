#include "fork.h"
#include "thread.h"
#include "debug.h"
#include "string.h"
#include "list.h"
#include "file.h"
#include "fs.h"
#include "process.h"
#include "page.h"
#include "stddef.h"
void copy_parent_pcb_to_child(struct task_struct *child_pcb, struct task_struct *parent_pcb) {
    assert(child_pcb && parent_pcb);
    assert(is_user_thread(parent_pcb));
    const enable_debug = true;
    memcpy(child_pcb, parent_pcb, PAGE_SIZE);
    // 一些需要修改的数据
    child_pcb->pid = allcoate_pid();
    child_pcb->parent_pid = parent_pcb->pid;

    if (enable_debug) {
        // 这里目前分配正常，但返回的时候就不对了，可能哪里写坏了
        printf("%s child_pcb->pid:%d\n", __FILE__, child_pcb->pid);
    }
    child_pcb->elapsed_ticks = 0;
    child_pcb->status = TASK_READY;
    child_pcb->ticks = child_pcb->priority;
    child_pcb->general_tag.next = NULL;
    child_pcb->general_tag.prev = NULL; 
    child_pcb->all_list_tag.next = NULL;
    child_pcb->all_list_tag.prev = NULL;
    
    // 我理解下应该不需要初始化，fork来的话，页表也是复制的
    // mem_block_desc_array_init(&child_pcb->u_block_desc);
    // 用户态的不需要，虚拟的用户空间可以实现堆的复制，操作的是不同的物理内存
    // fd_table，等于是重新打开了一份，给+1
    for (int i = USED_FD_START_INDEX; i < MAX_FILES_OPEN_PER_PROC; i++) {
        int32_t global_fd_index = child_pcb->fd_table[i];
        if (global_fd_index == -1) {
            continue;
        }

        if (g_file_table[global_fd_index].p_fd_inode) {
            g_file_table[global_fd_index].p_fd_inode->i_open_cnts++;
        }
    }
    
    // 用户态虚拟空间池本身的bits数据页需要额外分配一个,然后复制一下bits数据
    child_pcb->page_dir = create_page_dir();
    // 用户态的话，虚拟空间池初始化一下，主要是先分配一个bits数据页
    user_vmemory_pool_init(&child_pcb->vmemory_pool);
    vaddr_t child_bits = child_pcb->vmemory_pool.bmap.bits;
    // 然后复制下父进程的虚拟空间池的当前状态
    memcpy(&child_pcb->vmemory_pool, &parent_pcb->vmemory_pool, sizeof(struct memory_pool));
    // lock初始化
    mutex_init(&child_pcb->vmemory_pool.lock);
    // 复制bits数据
    child_pcb->vmemory_pool.bmap.bits = child_bits;
    memcpy(child_bits, parent_pcb->vmemory_pool.bmap.bits, DIV_ROUND_UP(parent_pcb->vmemory_pool.bmap.len_bit, 8));           
}

void copy_parent_user_sapce_data_to_child(struct task_struct *child_pcb, struct task_struct *parent_pcb) {
    assert(child_pcb && parent_pcb);
    assert(is_user_thread(parent_pcb));
    const bool enable_debug = false;
    // 如果是用户进程的话vmemory_pool在copy_pcb以及复制过了，这里只需要管数据和页表即可
    // 分配一个页用于数据中转
    Byte *page_buff = sys_malloc_in_kernel(PAGE_SIZE);
    // for(int i = 0; i < PAGE_SIZE; i++) {
    //     printf("%s page_buff[%d]=%d;\n",__FILE__,i,i);
    //     page_buff[i] = i;
    // }
    assert(page_buff);
    uint32_t parent_page_dir_entry_attr;
    uint32_t parent_page_table_entry_attr;

    vaddr_t user_memory_end_vaddr = parent_pcb->vmemory_pool.start + parent_pcb->vmemory_pool.length;
    if (enable_debug) {
         printf("%s parent_pcb->vmemory_pool.start:0x%x length:0x%x max_vaddr:0x%x\n", __FILE__, parent_pcb->vmemory_pool.start , parent_pcb->vmemory_pool.length, user_memory_end_vaddr);
    }

    for (int i = 0;i < parent_pcb->vmemory_pool.bmap.len_bit; i++) {
        bit_state state = bitmap_get(&parent_pcb->vmemory_pool.bmap, i);
        if (state == BIT_STATE_UNUSE) {
            continue;
        }
        vaddr_t user_page_vaddr = parent_pcb->vmemory_pool.start + i * PAGE_SIZE;
        // bitmap最后几位可能超过范围，这里判断下
        if (user_page_vaddr >= user_memory_end_vaddr) {
            continue;
        }
        // 激活父进程页表
        page_dir_activate(parent_pcb);
        // 复制下父进程的页表属性
        // 先看下页表的属性，如果不存在的话也跳过，感觉有些地方还是有点不同步
        if (enable_debug) {
            printf("%s i:%d will get_page_dir_entry_vaddr:0x%x page_dir paddr:0x%x\n", __FILE__, i,user_page_vaddr, parent_pcb->page_dir);
        }
        page* p_page_dir_entry = get_page_dir_entry_vaddr(user_page_vaddr, PAGE_DIR_VADDR);
        parent_page_dir_entry_attr = *(uint32_t *)p_page_dir_entry & 0xFFF;
        if (p_page_dir_entry->P == PAGE_P_VALUE_UNEXIST) {
            continue;
        }

        page *p_page_table_entry = get_page_table_entry_vaddr(user_page_vaddr, PAGE_DIR_VADDR);
        parent_page_table_entry_attr = *(uint32_t *)p_page_table_entry & 0xFFF;
        if (p_page_table_entry->P == PAGE_P_VALUE_UNEXIST) {
            continue;
        } 

        // 复制数据到缓冲区
        if (enable_debug) {
            printf("%s i:%d will copy 0x%x data to buff:0x%x\n", __FILE__, i, user_page_vaddr, page_buff);
        }
        memcpy(page_buff, user_page_vaddr, PAGE_SIZE);
        
 
        // 激活子进程页表
        page_dir_activate(child_pcb);
        // 看下子进程的页表如果没有就从父进程开始复制
        p_page_dir_entry = get_page_dir_entry_vaddr(user_page_vaddr, PAGE_DIR_VADDR);
        p_page_table_entry = get_page_table_entry_vaddr(user_page_vaddr, PAGE_DIR_VADDR);
        if(p_page_dir_entry->P == PAGE_P_VALUE_UNEXIST){
            //说明没有对应的页表，映射一个页表
            paddr_t alloced_page = (paddr_t)pmalloc_page(1);
            assert(alloced_page);
            if (enable_debug) {
                printf("%s i:%d will set_page_dir_entry 0x%x data to paddr:0x%x\n", __FILE__,i, user_page_vaddr, alloced_page);
            }
            set_page_dir_entry(user_page_vaddr, alloced_page, PAGE_DIR_VADDR, parent_page_dir_entry_attr);
        }
        if (p_page_table_entry->P == PAGE_P_VALUE_UNEXIST) {
            paddr_t alloced_page = (paddr_t)pmalloc_page(1);
            assert(alloced_page);
            if (enable_debug) {
                printf("%s i:%d will set_page_table_entry 0x%x data to paddr:0x%x\n", __FILE__,i, user_page_vaddr, alloced_page);
            }
            set_page_table_entry(user_page_vaddr, alloced_page, PAGE_DIR_VADDR, parent_page_table_entry_attr);
        }
        // 重新激活刷新下硬件缓存
        page_dir_activate(child_pcb);
        // 复制数据
        if (enable_debug) {
            printf("%s i:%d will copy buff:0x%x data to 0x%x\n", __FILE__, i, page_buff, user_page_vaddr);
        }
        memcpy(user_page_vaddr, page_buff, PAGE_SIZE);
    }
    sys_free_in_kernel(page_buff);
    page_dir_activate(parent_pcb);
}

NAKEDFUNC static void intr_exit(void) {
    asm volatile(" \
        addl $4, %esp; \
        popa; \
        pop %gs; \
        pop %fs; \
        pop %es; \
        pop %ds; \
        addl $4,%esp; \
        iret; \
    "); 
}

void adjust_copyed_child_pcb_stack(struct task_struct *child_pcb) {
    assert(is_user_thread(child_pcb));
    // 先定位中断栈
    struct interrupt_stack *p_interrupt_stack = (Byte *)child_pcb + PAGE_SIZE - sizeof(struct interrupt_stack);
    uint32_t *intr_0_stack = p_interrupt_stack;
    // 先按书上的来
     uint32_t* ret_addr_in_thread_stack  = (uint32_t*)intr_0_stack - 1;

   /***   这三行不是必要的,只是为了梳理thread_stack中的关系 ***/
   uint32_t* esi_ptr_in_thread_stack = (uint32_t*)intr_0_stack - 2; 
   uint32_t* edi_ptr_in_thread_stack = (uint32_t*)intr_0_stack - 3; 
   uint32_t* ebx_ptr_in_thread_stack = (uint32_t*)intr_0_stack - 4; 
   /**********************************************************/

   /* ebp在thread_stack中的地址便是当时的esp(0级栈的栈顶),
   即esp为"(uint32_t*)intr_0_stack - 5" */
   uint32_t* ebp_ptr_in_thread_stack = (uint32_t*)intr_0_stack - 5; 

   /* switch_to的返回地址更新为intr_exit,直接从中断返回 */
   *ret_addr_in_thread_stack = (uint32_t)intr_exit;

   /* 下面这两行赋值只是为了使构建的thread_stack更加清晰,其实也不需要,
    * 因为在进入intr_exit后一系列的pop会把寄存器中的数据覆盖 */
   *ebp_ptr_in_thread_stack = *ebx_ptr_in_thread_stack =\
   *edi_ptr_in_thread_stack = *esi_ptr_in_thread_stack = 0;
   /*********************************************************/

   /* 把构建的thread_stack的栈顶做为switch_to恢复数据时的栈顶 */
   child_pcb->self_kernel_stack = ebp_ptr_in_thread_stack;	    
   
    /*
    // 自己写的，有点问题
    // 再拉一个线程栈
    struct thread_stack *p_thread_stack = (Byte *)p_interrupt_stack - sizeof(struct thread_stack);
    // 子进程里的这个返回0
    p_interrupt_stack->eax = 0;
    // 填充intr_exit_from, 用intr_exit_from返回
    p_thread_stack->eip = kernel_thread;
    p_thread_stack->function = intr_exit_from;
    p_thread_stack->func_arg = p_interrupt_stack;
    p_thread_stack->ebp = 0;
    p_thread_stack->ebx = 0;
    p_thread_stack->esi = 0;
    p_thread_stack->edi = 0;
    */
}

void copy_process(struct task_struct *child_pcb, struct task_struct *parent_pcb) {
    assert(child_pcb && parent_pcb);
    assert(is_user_thread(parent_pcb));

    copy_parent_pcb_to_child(child_pcb, parent_pcb);
    copy_parent_user_sapce_data_to_child(child_pcb, parent_pcb);
    adjust_copyed_child_pcb_stack(child_pcb);
}

pid_t sys_fork() {
    struct task_struct* child_pcb = malloc_kernel_page(1);
    if (!child_pcb) {
        printf("%s malloc pcb faild\n", __FILE__);
        return -1;
    }
    memset(child_pcb, 0, PAGE_SIZE);
    struct task_struct* parent_pcb = get_current_pcb();
    interrupt_state old_state = close_interrupt();
    copy_process(child_pcb, parent_pcb);
    // 然后放进队列里面
    list_push_back(&thread_ready_list, &child_pcb->general_tag);
    list_push_back(&thread_all_list, &child_pcb->all_list_tag);
    set_interrupt_state(old_state);
    // printf("%s return pid:%d\n", __FILE__, child_pcb->pid);
    return child_pcb->pid;
}