#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "stdint.h"
#include "list.h"
#include "stddef.h"
#include "memory.h"
#define STACK_OVERFLOW_MAGIC_NUM 0xCCCCCCCC
// 单个进程最大的文件描述符个数
#define MAX_FILES_OPEN_PER_PROC 8
//通用线程函数类型
typedef void thread_func(void*);
//该类型的指针
typedef thread_func* p_thread_func;
typedef uint32_t pid_t;
typedef enum task_status{
    TASK_RUNNING, //运行状态
    TASK_READY, //就绪状态
    TASK_BLOCKED, //阻塞状态
    TASK_WAITING, 
    TASK_HANGING, 
    TASK_DIED
} task_status;




/***********   中断栈intr_stack   ***********
 * 此结构用于中断发生时保护程序(线程或进程)的上下文环境:
 * 进程或线程被外部中断或软中断打断时,会按照此结构压入上下文
 * 寄存器,  intr_exit中的出栈操作是此结构的逆操作
 * 此栈在线程自己的内核栈中位置固定,所在页的最顶端
********************************************/
struct interrupt_stack {
    uint32_t interrupt_num;	 //统一中断栈架构的中断号，部分没有中断号的中断会压入一个0，用以统一中断栈结构
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;	 // 虽然pushad把esp也压入,但esp是不断变化的,所以这个值其实没什么用，即使弹出到了esp也马上会被后续的pop过程修改
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

/* 以下由cpu从低特权级进入高特权级时压入 */
    uint32_t err_code;		 // err_code会被压入在eip之后
    void (*eip) (void);
    uint32_t cs;
    uint32_t eflags;
    void* esp;
    uint32_t ss;

};

/***********  线程栈thread_stack  ***********
 * 线程自己的栈,用于存储线程中待执行的函数
 * 此结构在线程自己的内核栈中位置不固定,
 * 用在switch_to时保存线程环境。
 * 实际位置取决于实际运行情况。
 ******************************************/
struct thread_stack {
   /*这些寄存器ebp,ebx,edi,esi和 esp 归主调函数所用，其余的寄存器归被调函数所用 
   换句话说，不管被调函数中是否使用了这个寄存器，在被调函数执行完后，这个寄存器的值不该被改变 
   因此被调函数必须为主调函数保护好这个寄存器的值，
   在被调函数运行完之后，这个寄存器的值必须和运行前一样，它必须在自己的栈中存储这些寄存器的值*/
   uint32_t ebp;
   uint32_t ebx;
   uint32_t edi;
   uint32_t esi;
   //uint32_t esp 不需要保存这个，这个随栈的变化保证

/* 线程第一次执行时,eip指向待调用的函数kernel_thread 
其它时候,eip是指向switch_to的返回地址*/
   void (*eip) (thread_func* func, void* func_arg);

/*****   以下仅供第一次被调度上cpu时使用   ****/

/* 参数unused_ret只为占位置充数为返回地址 */
   void (*unused_retaddr);
   p_thread_func function;   // 由Kernel_thread所调用的函数名
   void* func_arg;    // 由Kernel_thread所调用的函数所需的参数
};

// PCB新增数据需要make clean一下，不然有些地方会报错，可能是遗留了之前内存布局的编译文件没有改
/* 进程或线程的pcb,程序控制块 */
struct task_struct {
   vaddr_t self_kernel_stack;	 // 各内核线程都用自己的内核栈, 基本上是指向PCB高地址的地方, 也即存ESP的位置
   pid_t pid;
   struct mem_block_desc u_block_desc[BLOCK_DESC_SIZE];
   enum task_status status;
   uint8_t priority;		 // 线程优先级, 目前的用法，越大优先级越高
   char name[16];

   uint8_t ticks;//剩余可运行的时钟滴答数
   uint32_t elapsed_ticks;//总共运行了的时种滴答数
   int32_t fd_table[MAX_FILES_OPEN_PER_PROC]; // 文件描述符数组
   struct list_node general_tag;//一般队列中的节点
   struct list_node all_list_tag;//总队列中的节点
   vaddr_t page_dir; //页目录地址,因为能访问到这个变量的时候已经是保护模式了，所以是虚拟地址
   memory_pool vmemory_pool; // 用于标记虚拟地址空间池, 只用于用户进程，内核用的是全局变量记录的
   uint32_t stack_magic;	 // 用这串数字做栈的边界标记,用于检测栈的溢出


/*
   PCB内存布局
   高
   -------
   中断栈
   ---------
   线程运行栈
   --------   <-----self_kernel_stack基本上指向这里
   栈可用的留空空间
   ---------
   PCB成员数据
   ---------
   低
*/
};


extern struct task_struct* main_thread_pcb; //主线程PCB，等会启动的时候切换到这个线程，保证模型一致
extern struct list thread_ready_list; //就绪队列
extern struct list thread_all_list;//总队列


/*
@brief 在初始化后的PCB上填充线程要运行的函数信息

*/
void thread_create(struct task_struct* pcb, thread_func function, void* func_arg);

/*
@brief 初始化pcb的基本信息
@param pcb: task_struct* :pcb指针
@param name: char *: 线程名
@param priority: int : 线程优先级
*/
void init_pcb(struct task_struct* pcb, char* name, int priority);


/*
@brief 运行一个线程
@param name: char* :不超过15字节的名称
@param priority: int :优先级
@param function: thread_func :要运行的函数
@param func_arg: void* :函数参数

*/
struct task_struct* thread_start(char* name, int priority, thread_func function, void* func_arg);

/*
@brief 获取当前运行线程的PCB地址，原理是因为线程运行栈和PCB都在一个页上，且页的最低地址就是PCB的首地址
@return struct task_struct*: PCB地址
*/
struct task_struct* get_current_pcb(void);


/*
@brief 初始化线程运行的控制信息并运行一个最初的线程
@param main_function: thread_func :要运行的函数
@param func_arg: void* :函数参数
*/
void init_thread_boot(thread_func main_function, void* func_arg);


/*
@brief 线程调度函数，调用前需要关中断，使用后要开中断
*/
void schedule();

/*
@brief 阻塞当前线程，并将状态设置为status,只能用 TASK_BLOCKED , TASK_HANGING, TASK_WAITING这三种状态
@param status: task_status : 阻塞后的线程状态
*/
void thread_block(task_status status);

/*
@brief 解除某个线程的阻塞状态
@param pcb: struct task_struct* : 某个线程的PCB指针
*/
void thread_unblock(struct task_struct* pcb);

/*
@brief 当前线程主动放弃CPU
*/
void thread_yield();


/*
@brief 分配一个未使用的PID 本质是按序分配
@return pid_t 未使用的pid
*/
pid_t allcoate_pid(void);


/*
@brief 判断一个PCB对应的是不是内核线程
@param pcb: struct task_struct* : PCB地址
@return bool true则是内核态，否则用户态
*/
bool is_kernel_thread(struct task_struct* pcb);


/*
@brief 判断一个PCB对应的是不是内核线程
@param pcb: struct task_struct* : PCB地址
@return bool true则是用户态，否则是内核态
*/
bool is_user_thread(struct task_struct* pcb);

/*
@brief idle线程函数
*/
void idle_thread_func(void *args);


/*
@brief 主线程函数
*/
void main_thread_func(void *args);


/*
@brief 初始化idle线程的相关结构
*/
void init_idle_thread();
#endif