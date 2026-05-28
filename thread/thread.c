#include "thread.h"
#include "page.h"
#include "string.h"
#include "interrupt.h"
#include "memory.h"
#include "debug.h"
#include "io.h"
#include "process.h"
#include "list.h"
#include "fs.h"
#include "syscall.h"
#include "dir.h"
#include "test_thread.h"
#include "file.h"
#include "shell.h"

#define MIN_TICKS 1
struct task_struct* main_thread_pcb; //主线程PCB，等会启动的时候切换到这个线程，保证模型一致
struct task_struct* idle_thread_pcb; //idle空闲进程
struct list thread_ready_list; //就绪队列
struct list thread_all_list;//总队列
struct mutex pid_lock;

/* 由kernel_thread去执行function(func_arg) */
void kernel_thread(thread_func* function, void* func_arg) {
/* 执行function前要开中断,避免后面的时钟中断被屏蔽,而无法调度其它线程 */
   open_interrupt();
   //发送中断结束命令EOI,不然从中断切换线程后，E820不会再发时钟中断，等价于没开中断，会一直运行切换后的线程
   outb(0xa0, 0x20);
   outb(0x20, 0x20);
   const bool enable_debug = false;
   if (enable_debug) {
    struct task_struct *pcb = get_current_pcb();
    printf("%s pid:%d\n", __FILE__, pcb->pid);
   }
   function(func_arg); 
}

NAKEDFUNC static void switch_to(struct task_struct* cur, struct task_struct* next){
    //此时的栈
    /*高地址， 一行代表4字节
    next
    cur
    old_eip
    低地址*/
    //调用约定保证EAX可以直接用
    asm volatile(" \
    push %esi; \
    push %edi; \
    push %ebx; \
    push %ebp; \
    movl 20(%esp), %eax; \
    movl %esp, (%eax); \
    "); //cur = [esp+20]
    //上面代码主要作用就是 cur.self_kernel_stack = now_esp 
    /*高地址， 一行代表4字节
    next
    cur (主要是访问这个self_kernel_stack，地址和cur一样,毕竟是第一个成员变量)
    old_eip
    old_esi
    old_edi
    old_ebx
    old_ebp  <---esp
    低地址*/

   asm volatile(" \
    movl 24(%esp), %eax; \
    movl (%eax), %esp; \
    pop %ebp; \
    pop %ebx; \
    pop %edi; \
    pop %esi; \
    ret; \
    "); 
    //上面这段代码主要作用就是 now_esp = next.self_kernel_stack
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
    pcb->pid = allcoate_pid();
    pcb->parent_pid = -1;
    if(pcb == main_thread_pcb){
        pcb->status = TASK_RUNNING;
    }
    else{
        pcb->status = TASK_READY;
    }
    
    pcb->self_kernel_stack = (vaddr_t)((uint32_t)pcb + PAGE_SIZE);

    // 预留的标准输入输出初始化为正确的值，其他为-1
    for (int i = 0; i < MAX_FILES_OPEN_PER_PROC; i++) {
        pcb->fd_table[i] = i < 3 ? i : -1;
    }
    pcb->current_workdir_inode_no = 0; //默认根目录
    pcb->stack_magic = STACK_OVERFLOW_MAGIC_NUM;
}


struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg){
    struct task_struct* pcb;
    if(idle_thread_func == function){
        pcb = idle_thread_pcb;
    }
    else{
        pcb = malloc_kernel_page(1);
    }
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
    mutex_init(&pid_lock);
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
    //不在内部关中断，是因为用这个函数的时候在外部关，换线程后，还有机会换回来，然后在外部开
    assert(get_interrupt_state() == INTERRUPT_DISABLE);
    const enable_debug = false;
    if (enable_debug) {
        debug("schedul begin\n");
    }
   struct task_struct* cur = get_current_pcb(); 
   if (cur->status == TASK_RUNNING) { // 若此线程只是cpu时间片到了,将其加入到就绪队列尾
        cur->ticks = cur->priority;     // 重新将当前线程的ticks再重置为其priority;
        cur->status = TASK_READY;
        list_push_back(&thread_ready_list, &cur->general_tag);
   } else { 
      /* 若此线程需要某事件发生后才能继续上cpu运行,
      不需要将其加入队列,因为当前线程不在就绪队列中。*/
   }
    
   if(list_empty(&thread_ready_list)){
        //debug("schedule unblock idle\n");
        thread_unblock(idle_thread_pcb);
   }
   
   struct list_node* thread_tag = list_pop_front(&thread_ready_list);   
   struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);

    
   // clear_screen();
   if (enable_debug) {
        debug("%x switch to %x pid:%d->%d\n", cur, next, cur->pid, next->pid);
   }
   next->status = TASK_RUNNING;
   if (enable_debug) {
        debug("before activate cr3:%x\n",get_cr3_register());
   }
   process_activate(next);
   if (enable_debug) {
        debug("after activate cr3:%x\n",get_cr3_register());
        debug("schedul end next call switch_to\n");
   }
   switch_to(cur, next);
}


void thread_block(task_status status){
    //只能取这三种状态
    assert(status == TASK_BLOCKED || status == TASK_HANGING || status == TASK_WAITING);
    interrupt_state old_state = close_interrupt();
    struct task_struct* pcb = get_current_pcb();
    pcb->status = status;
    schedule();
    set_interrupt_state(old_state);
}



void thread_unblock(struct task_struct* pcb){
    /*
    static size_t call_count = 0;
    call_count++;    
    debug("thread_unblock call_count:%d\n",call_count);
    */
    //debug("thread_unblock pcb->status:%d name:%s addr:%x\n", pcb->status,pcb->name,pcb);
    assert(pcb->status == TASK_BLOCKED || pcb->status == TASK_HANGING || pcb->status == TASK_WAITING);
    interrupt_state old_state = close_interrupt();
    if(pcb->status != TASK_READY){
        assert(!find_node(&thread_ready_list, &pcb->general_tag));
        pcb->status = TASK_READY;
        pcb->ticks = pcb->priority;
        list_push_back(&thread_ready_list, &pcb->general_tag);
    }
    set_interrupt_state(old_state);
}

void sys_thread_yield(){
    struct task_struct* pcb = get_current_pcb();
    interrupt_state old_state = close_interrupt();
    assert(pcb->status == TASK_RUNNING);
    assert(!find_node(&thread_ready_list, &pcb->general_tag));
    list_push_back(&thread_ready_list, &pcb->general_tag);
    pcb->status = TASK_READY;
    schedule();
    set_interrupt_state(old_state); 
}

pid_t allcoate_pid(void){
    static pid_t next_pid = 0;
    pid_t ret = 0;
    while(1){
        lock(&pid_lock);
        ret = next_pid++;
        unlock(&pid_lock);
        //找下PID是否已经存在，存在就重新分配
        bool ret_exist_flag = false;
        interrupt_state old_state = close_interrupt();
        if(!list_empty(&thread_all_list)){
            struct list_node* iter = thread_all_list.head.next;
            while(iter != &(thread_all_list.tail)){
                struct task_struct* pcb = elem2entry(struct task_struct, all_list_tag, iter); 
                if(pcb->pid == ret){
                    ret_exist_flag = true;
                    break;
                }
                iter = iter->next;
            }
        }
        set_interrupt_state(old_state);  
        if(!ret_exist_flag){
            break;
        }
    }
    return ret;
}

bool is_kernel_thread(struct task_struct* pcb){
    return NULL == pcb->page_dir;
}

bool is_user_thread(struct task_struct* pcb){
    return NULL != pcb->page_dir;
}

void idle_thread_func(void *args){
    while(1){
        thread_block(TASK_BLOCKED);
        open_interrupt();
        sys_wait(NULL);
        //hlt使CPU停转，等待中断响应 
        asm volatile("hlt;":::"memory");
    }
}

void main_thread_func(void *args){
    close_interrupt();
    init_idle_thread();
    open_interrupt();
    init_other_in_main_thread();
    // 其他测试逻辑
    // 测试关中断的话磁盘还能读取吗,结论可以
    // close_interrupt();
    struct Dir *p_dir = sys_opendir("/");
    struct Dir_entry *p_dir_entry;
    int count = 0;
    while(p_dir_entry = readdir(p_dir)) {
        count++;
        printf("readdir p_dir_entry:0x%x name:%s type:%d \n", p_dir_entry, p_dir_entry->fileName, p_dir_entry->f_type);
    }
  
    for (int i = 0; i < 5; i++) {
        char path[10] = {'/', 'a', 0};
        path[1] += i;
        if (count <= 2) {
            int32_t res = sys_mkdir(path);
            printf("sys_mkdir:%s res:%d\n",path, res);
        } else {
            //int32_t res = sys_rmdir(path);
            //printf("sys_rmdir:%s res:%d\n",path, res);
        }
        struct Stat stat_data = {0};
        int res = stat(path, &stat_data);
        printf("path:%s stat res:%d inode_no:%d file_type:%d size:%d\n", path, res, stat_data.st_inode_no, stat_data.st_file_type, stat_data.st_size);
    }

    
    char *buff = malloc(MAX_PATH_LENGTH);
    assert(buff);
    getcwd(buff, MAX_PATH_LENGTH);
    printf("getcwd:%s\n", buff);

    printf("chdir /b :%d\n",chdir("/b"));
    char *res = getcwd(buff, MAX_PATH_LENGTH);
    printf("getcwd:0x%x %s res:0x%x %s\n",buff, buff,res, res);

    memset(buff, 0, MAX_PATH_LENGTH);
    // read(stdin_no, buff, 20);
    printf("buff read:%s\n", buff);
    
    free(buff);
    

    process_execute(my_shell, "my_shell");
    // my_shell();
    // process_execute(test_fork, "test_fork");
    // int res = sys_mkdir("/a");
    // printf("call sys_mkdir res:%d\n", res);
    // char *fileName = "/e2.txt";
    // printf("call sys_unlink %s\n", fileName);
    // int res = sys_unlink(fileName);
    // printf("sys_unlink %s res:%d\n", fileName, res);
    
    // int fd = sys_open(fileName, O_CREAT | O_RDWR);
    // printf("sys_open first open fd:%d\n", fd);
    // if (fd == -1) {
    //     fd = sys_open(fileName, O_RDWR);
    // }
    // printf("sys_open %s fd:%d\n",fileName, fd);

    // char * textStr = "hello sys_write\n";
    // write(fd, textStr, strlen(textStr));

    // // textStr =  "second call sys_write\n";
    // // sys_write(fd, textStr, strlen(textStr));
    // char *buff = malloc(BLOCK_SIZE);
    
    // memset(buff, 0, BLOCK_SIZE);
    // int read_count = read(fd, buff, BLOCK_SIZE);
    // while(-1 != read_count) {
    //     printf("read count:%d content:%s\n",read_count, buff);
    //     memset(buff, 0, BLOCK_SIZE);
    //     read_count = read(fd, buff, BLOCK_SIZE);
    // }
    

    while(1){}
}

void init_idle_thread(){
    thread_start("idle",1,idle_thread_func,NULL);
}


static bool sys_ps_traversal_func(struct list_node* node, int arg) {
    struct task_struct *pcb = elem2entry(struct task_struct,all_list_tag, node);
    int buff_length = 17;
    char buff[17] = {0};
    buff[buff_length - 1] = 0;
    memset(buff, ' ', buff_length - 1);
    sprintf(buff, "%d", pcb->pid);
    printf("%s", buff);

    memset(buff, ' ', buff_length - 1);
    sprintf(buff, "%d", pcb->parent_pid);
    printf("%s", buff);

    memset(buff, ' ', buff_length - 1);
    char *stat_str[] = {
    "RUNNING",
    "READY", 
    "BLOCKED", 
    "WAITING", 
    "HANGING", 
    "DIED"
    };
    sprintf(buff, "%s", stat_str[pcb->status]);
    printf("%s", buff);

    memset(buff, ' ', buff_length - 1); 
    sprintf(buff, "%d", pcb->ticks);
    printf("%s", buff);

    memset(buff, ' ', buff_length - 1); 
    sprintf(buff, "%s", pcb->name);
    printf("%s\n", buff);

    return false;
}

void sys_ps() {
    printf("\nPID             PPID           STAT            TICKS           COMMAND         \n");
    // 打印当前所有的进程信息
    interrupt_state old_state = close_interrupt();
    list_traversal(&thread_all_list, sys_ps_traversal_func, 0);
    set_interrupt_state(old_state);
}

pid_t sys_wait(int32_t *p_exit_status) {
    interrupt_state old_state = close_interrupt();

    struct task_struct* pcb = get_current_pcb();
    pid_t ret_child_pid = -1;
    bool has_child = false;
    do {
        // 先查找是否有子进程
        struct list_node* iter = thread_all_list.head.next;
        while(iter != &(thread_all_list.tail)){
            struct task_struct* iter_pcb = elem2entry(struct task_struct, all_list_tag, iter); 
            if (iter_pcb->parent_pid == pcb->pid) {
                has_child = true;
                if (iter_pcb->status == TASK_DIED) {
                    ret_child_pid = iter_pcb->pid;
                    if (p_exit_status) {
                        *p_exit_status = iter_pcb->exit_status;
                    }
                    // 从列表里移除pcb
                    list_remove(&iter_pcb->all_list_tag);
                    // 回收pcb
                    free_kernel_page(iter_pcb, 1);
                    break;
                }
            }
            iter = iter->next;
        }
        if (ret_child_pid != -1 || !has_child) {
            break;
        }
        set_interrupt_state(old_state);
        sys_thread_yield();
        old_state = close_interrupt();
    } while(has_child && ret_child_pid == -1);
    set_interrupt_state(old_state);
    return ret_child_pid;
}

// 回收PCB的空间资源
static void release_pcb_prog_resource(struct task_struct *pcb) {
    // 遍历页表回收空间资源
    for (uint32_t user_page_start_vaddr = 0; user_page_start_vaddr < 0xC0000000; user_page_start_vaddr += PAGE_SIZE * 1024) {
        page* p_page_dir_entry = get_page_dir_entry_vaddr(user_page_start_vaddr, PAGE_DIR_VADDR);
        if (p_page_dir_entry->P == PAGE_P_VALUE_UNEXIST) {
            continue;
        }

        for (uint32_t user_page_vaddr = user_page_start_vaddr;user_page_vaddr < user_page_start_vaddr +  PAGE_SIZE * 1024; user_page_vaddr += PAGE_SIZE) {
            page *p_page_table_entry = get_page_table_entry_vaddr(user_page_vaddr, PAGE_DIR_VADDR);
            if (p_page_table_entry->P == PAGE_P_VALUE_UNEXIST) {
                continue;
            } 
            pfree_page(p_page_table_entry->PADDR * PAGE_SIZE, 1);
        }
        pfree_page(p_page_dir_entry->PADDR * PAGE_SIZE, 1);
    }
    vaddr_t page_dir = pcb->page_dir;
    pcb->page_dir = NULL;
    // 切换为内核页表
    page_dir_activate(pcb);
    // 回收用户空间的页表内存
    free_kernel_page(page_dir, 1);

    // 然后回收虚拟内存池的位图空间
    size_t pool_page_count = (USER_PROCESS_MEMORY_MAX_LENGTH + PAGE_SIZE - 1)/ PAGE_SIZE;
    free_kernel_page(pcb->vmemory_pool.bmap.bits, pool_page_count);
}

void sys_exit(int32_t exit_status) {
    interrupt_state old_state = close_interrupt();
    struct task_struct *pcb = get_current_pcb();
    // 先从运行链表里移除
    list_remove(&pcb->general_tag);
    // 设置退出状态
    pcb->status = TASK_DIED;
    pcb->exit_status = exit_status;
    // 然后遍历一下找出子进程，让init收养
    struct list_node* iter = thread_all_list.head.next;
    while(iter != &(thread_all_list.tail)){
        struct task_struct* iter_pcb = elem2entry(struct task_struct, all_list_tag, iter); 
        if (iter_pcb->parent_pid == pcb->pid) {
            iter_pcb->parent_pid = idle_thread_pcb->pid;
        }
        iter = iter->next;
    }
    // 回收其他空间
    release_pcb_prog_resource(pcb);
    set_interrupt_state(old_state);
    schedule();
}