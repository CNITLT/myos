#include "fock.h"
#include "thread.h"
#include "debug.h"
#include "string.h"
#include "list.h"
#include "file.h"
#include "fs.h"
#include "process.h"
#include "page.h"

void copy_parent_pcb_to_child(struct task_struct *child_pcb, struct task_struct *parent_pcb) {
    assert(child_pcb && parent_pcb);

    memcpy(child_pcb, parent_pcb, sizeof(struct task_struct));
    // 一些需要修改的数据
    child_pcb->pid = allcoate_pid();
    child_pcb->parent_pid = parent_pcb->pid;

    child_pcb->elapsed_ticks = 0;
    child_pcb->status = TASK_READY;
    child_pcb->ticks = child_pcb->priority;
    child_pcb->general_tag.next = NULL;
    child_pcb->general_tag.prev = NULL; 
    child_pcb->all_list_tag.next = NULL;
    child_pcb->all_list_tag.prev = NULL;
    
    // 我理解下应该不需要初始化，fork来的话，页表也是复制的
    // 对于内核进程的fork需要重新初始化，不然，因为这个demo里内核进程的堆是共享的，如果这里不初始化，会有问题
    if (is_kernel_thread(parent_pcb)) {
        // 内核进程这里还是初始化一下，免得父子进程操作的堆都一样，虽然理论上来讲子进程此时不应该去操作堆的内存了
        // 因为理论上都是父进程来管理的，什么时候释放也不好说
        mem_block_desc_array_init(&child_pcb->u_block_desc);
    }
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
    if (!is_kernel_thread(parent_pcb)) {
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
}

void copy_parent_user_sapce_data_to_child(struct task_struct *child_pcb, struct task_struct *parent_pcb) {
    if (is_kernel_thread(parent_pcb)) {
        return;
    }
    // 如果是用户进程的话vmemory_pool在copy_pcb以及复制过了，这里只需要管数据和页表即可
    // 分配一个页用于数据中转
    Byte *page_buff = sys_malloc_in_kernel(PAGE_SIZE);
    uint32_t parent_page_dir_entry_attr;
    uint32_t parent_page_table_entry_attr;

    vaddr_t user_memory_end_vaddr = parent_pcb->vmemory_pool.start + parent_pcb->vmemory_pool.length;
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
        // 复制数据到缓冲区
        memcpy(page_buff, user_page_vaddr, PAGE_SIZE);
        // 复制下父进程的页表属性
        page* p_page_dir_entry = get_page_dir_entry_vaddr(user_page_vaddr, parent_pcb->page_dir);
        parent_page_dir_entry_attr = *(uint32_t *)p_page_dir_entry & 0xFFF;
        page *p_page_table_entry = get_page_table_entry_vaddr(user_page_vaddr, parent_pcb->page_dir);
        parent_page_table_entry_attr = *(uint32_t *)p_page_table_entry & 0xFFF;

        // 激活子进程页表
        page_dir_activate(child_pcb);
        // 看下子进程的页表如果没有就从父进程开始复制
        p_page_dir_entry = get_page_dir_entry_vaddr(user_page_vaddr, child_pcb->page_dir);
        p_page_table_entry = get_page_table_entry_vaddr(user_page_vaddr, child_pcb->page_dir);
        if(p_page_dir_entry->P == PAGE_P_VALUE_UNEXIST){
            //说明没有对应的页表，映射一个页表
            set_page_dir_entry(user_page_vaddr, (paddr_t)pmalloc_page(1), child_pcb->page_dir, parent_page_dir_entry_attr);
        }
        if (p_page_table_entry->P == PAGE_P_VALUE_UNEXIST) {
            set_page_table_entry(user_page_vaddr, (paddr_t)pmalloc_page(1), child_pcb->page_dir, parent_page_table_entry_attr);
        }
        // 重新激活刷新下硬件缓存
        page_dir_activate(child_pcb);
        // 复制数据
        memcpy(user_page_vaddr, page_buff, PAGE_SIZE);
    }
    sys_free_in_kernel(page_buff);
    page_dir_activate(parent_pcb);
}
