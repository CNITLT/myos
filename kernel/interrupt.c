#include "interrupt.h"
#include "print.h"
#include "io.h"
#include "stddef.h"
#include "thread.h"
#include "debug.h"
#include "init.h"
//中断门描述符定义
typedef struct interrupt_gate_desc{
    uint16_t func_offset_low; //中断处理程序在目标代码段的偏移中的低16位
    uint16_t code_selector; //目标代码段选择子
    struct {
        uint16_t fixed:11; //对中断门来说是固定的几位，高地址110 0000 0000低地址 
        uint16_t D:1; //D为1表示32位，为0是16位
        uint16_t S:1; //固定的为0
        uint16_t DPL:2; //特权级
        uint16_t P:1; //存在位
    } attribute;
    uint16_t func_offset_high; //中断处理程序在目标代码段的偏移中的高16位
} interrupt_gate_desc;

#define DEFAULT_INTERRUPT_GATE_DESC_VALUE {0,(1 << 3),{0x600,1,0,0,1},0}
#define IDT_SIZE 48


#define ERROR_CODE_RET asm volatile("addl $4,%esp;iret;");
#define DEFAULT_RET asm volatile("iret;");



//代理中断函数，主要用来处理返回到正确的地方,普通函数都是ret返回
#define IDT_FUNC_ENTRY_PROXY(INTERRUPT_NUM, RET_TYPE) \
NAKEDFUNC static void  IDT_FUNC_ENTRY_PROXY##INTERRUPT_NUM(void){ \
    asm volatile (" \
    push %ds; \
    push %es; \
    push %fs; \
    push %gs; \
    pusha;"); /*PUSHA指令压入32位寄存器,其入栈顺序是: EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI*/\
    /*put_int(INTERRUPT_NUM); */ \
    asm volatile(" \
    push %0 \
    "::"Ni"(INTERRUPT_NUM)); \
    asm volatile(" \
    call *%0; \
    "::"m"(IDT_FUNCS[INTERRUPT_NUM]), "Ni"(INTERRUPT_NUM)); \
    asm volatile(" \
    add $4, %esp \
    "); \
    asm volatile(" \
    push %eax; \
    movb $0x20, %al; \
    out %al,$0xA0; \
    out %al,$0x20; \
    pop %eax; \
    "); \
    asm volatile(" \
    popa; \
    pop %gs; \
    pop %fs; \
    pop %es; \
    pop %ds;"); \
    RET_TYPE \
} 

static interrupt_func_handler IDT_FUNCS[IDT_SIZE];//实际的处理函数


interrupt_gate_desc IDT[IDT_SIZE];
IDT_FUNC_ENTRY_PROXY(0,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(1,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(2,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(3,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(4,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(5,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(6,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(7,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(8,ERROR_CODE_RET)
IDT_FUNC_ENTRY_PROXY(9,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(10,ERROR_CODE_RET)
IDT_FUNC_ENTRY_PROXY(11,ERROR_CODE_RET)
IDT_FUNC_ENTRY_PROXY(12,ERROR_CODE_RET)
IDT_FUNC_ENTRY_PROXY(13,ERROR_CODE_RET)
IDT_FUNC_ENTRY_PROXY(14,ERROR_CODE_RET)
IDT_FUNC_ENTRY_PROXY(15,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(16,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(17,ERROR_CODE_RET)
IDT_FUNC_ENTRY_PROXY(18,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(19,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(20,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(21,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(22,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(23,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(24,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(25,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(26,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(27,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(28,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(29,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(30,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(31,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(32,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(33,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(34,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(35,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(36,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(37,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(38,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(39,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(40,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(41,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(42,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(43,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(44,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(45,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(46,DEFAULT_RET)
IDT_FUNC_ENTRY_PROXY(47,DEFAULT_RET)



interrupt_func_handler IDT_ENTRY_PROXYS[IDT_SIZE] = {
   IDT_FUNC_ENTRY_PROXY0,
   IDT_FUNC_ENTRY_PROXY1,
   IDT_FUNC_ENTRY_PROXY2,
   IDT_FUNC_ENTRY_PROXY3,
   IDT_FUNC_ENTRY_PROXY4,
   IDT_FUNC_ENTRY_PROXY5,
   IDT_FUNC_ENTRY_PROXY6,
   IDT_FUNC_ENTRY_PROXY7,
   IDT_FUNC_ENTRY_PROXY8,
   IDT_FUNC_ENTRY_PROXY9,
   IDT_FUNC_ENTRY_PROXY10,
   IDT_FUNC_ENTRY_PROXY11,
   IDT_FUNC_ENTRY_PROXY12,
   IDT_FUNC_ENTRY_PROXY13,
   IDT_FUNC_ENTRY_PROXY14,
   IDT_FUNC_ENTRY_PROXY15,
   IDT_FUNC_ENTRY_PROXY16,
   IDT_FUNC_ENTRY_PROXY17,
   IDT_FUNC_ENTRY_PROXY18,
   IDT_FUNC_ENTRY_PROXY19,
   IDT_FUNC_ENTRY_PROXY20,
   IDT_FUNC_ENTRY_PROXY21,
   IDT_FUNC_ENTRY_PROXY22,
   IDT_FUNC_ENTRY_PROXY23,
   IDT_FUNC_ENTRY_PROXY24,
   IDT_FUNC_ENTRY_PROXY25,
   IDT_FUNC_ENTRY_PROXY26,
   IDT_FUNC_ENTRY_PROXY27,
   IDT_FUNC_ENTRY_PROXY28,
   IDT_FUNC_ENTRY_PROXY29,
   IDT_FUNC_ENTRY_PROXY30,
   IDT_FUNC_ENTRY_PROXY31,
   IDT_FUNC_ENTRY_PROXY32,
   IDT_FUNC_ENTRY_PROXY33,
   IDT_FUNC_ENTRY_PROXY34,
   IDT_FUNC_ENTRY_PROXY35,
   IDT_FUNC_ENTRY_PROXY36,
   IDT_FUNC_ENTRY_PROXY37,
   IDT_FUNC_ENTRY_PROXY38,
   IDT_FUNC_ENTRY_PROXY39,
   IDT_FUNC_ENTRY_PROXY40,
   IDT_FUNC_ENTRY_PROXY41,
   IDT_FUNC_ENTRY_PROXY42,
   IDT_FUNC_ENTRY_PROXY43,
   IDT_FUNC_ENTRY_PROXY44,
   IDT_FUNC_ENTRY_PROXY45,
   IDT_FUNC_ENTRY_PROXY46,
   IDT_FUNC_ENTRY_PROXY47
}; //代理函数的地址


/* 
   前20个中断的意义
   intr_name[0] = "#DE Divide Error";
   intr_name[1] = "#DB Debug Exception";
   intr_name[2] = "NMI Interrupt";
   intr_name[3] = "#BP Breakpoint Exception";
   intr_name[4] = "#OF Overflow Exception";
   intr_name[5] = "#BR BOUND Range Exceeded Exception";
   intr_name[6] = "#UD Invalid Opcode Exception";
   intr_name[7] = "#NM Device Not Available Exception";
   intr_name[8] = "#DF Double Fault Exception";
   intr_name[9] = "Coprocessor Segment Overrun";
   intr_name[10] = "#TS Invalid TSS Exception";
   intr_name[11] = "#NP Segment Not Present";
   intr_name[12] = "#SS Stack Fault Exception";
   intr_name[13] = "#GP General Protection Exception";
   intr_name[14] = "#PF Page-Fault Exception";
   // intr_name[15] 第15项是intel保留项，未使用
   intr_name[16] = "#MF x87 FPU Floating-Point Error";
   intr_name[17] = "#AC Alignment Check Exception";
   intr_name[18] = "#MC Machine-Check Exception";
   intr_name[19] = "#XF SIMD Floating-Point Exception";
*/
static const interrupt_gate_desc default_value = DEFAULT_INTERRUPT_GATE_DESC_VALUE;

/*
@brief 中断向量表的初始化
*/
static void idt_desc_init(void){
    for(int i = 0; i < IDT_SIZE; i++){
        IDT[i] = default_value;
        IDT[i].func_offset_low = (uint32_t)IDT_ENTRY_PROXYS[i] & 0x0000FFFF;
        IDT[i].func_offset_high = ((uint32_t)IDT_ENTRY_PROXYS[i] & 0xFFFF0000) >> 16; 
    }
}

#define PIC_8259A_MASTER_ICW1_PORT 0x20
#define PIC_8259A_MASTER_ICW2_PORT 0x21
#define PIC_8259A_MASTER_ICW3_PORT 0x21
#define PIC_8259A_MASTER_ICW4_PORT 0x21

#define PIC_8259A_SLAVE_ICW1_PORT 0xA0
#define PIC_8259A_SLAVE_ICW2_PORT 0xA1
#define PIC_8259A_SLAVE_ICW3_PORT 0xA1
#define PIC_8259A_SLAVE_ICW4_PORT 0xA1

#define PIC_8259A_MASTER_OCW1_PORT 0x21
#define PIC_8259A_MASTER_OCW2_PORT 0x20
#define PIC_8259A_MASTER_OCW3_PORT 0x20

#define PIC_8259A_SLAVE_OCW1_PORT 0xA1
#define PIC_8259A_SLAVE_OCW2_PORT 0xA0
#define PIC_8259A_SLAVE_OCW3_PORT 0xA0

/*
@brief 可编程中断控制器的初始化，这里是8259A
*/
static void pic_init(void){
    
   /* 初始化主片 */
   outb (PIC_8259A_MASTER_ICW1_PORT, 0x11);   // ICW1: 边沿触发,级联8259, 需要ICW4.
   outb (PIC_8259A_MASTER_ICW2_PORT, 0x20);   // ICW2: 起始中断向量号为0x20,也就是IR[0-7] 为 0x20 ~ 0x27.
   outb (PIC_8259A_MASTER_ICW3_PORT, 0x04);   // ICW3: IR2接从片. 
   outb (PIC_8259A_MASTER_ICW4_PORT, 0x01);   // ICW4: 8086模式, 正常EOI

   /* 初始化从片 */
   outb (PIC_8259A_SLAVE_ICW1_PORT, 0x11);    // ICW1: 边沿触发,级联8259, 需要ICW4.
   outb (PIC_8259A_SLAVE_ICW2_PORT, 0x28);    // ICW2: 起始中断向量号为0x28,也就是IR[8-15] 为 0x28 ~ 0x2F.
   outb (PIC_8259A_SLAVE_ICW3_PORT, 0x02);    // ICW3: 设置从片连接到主片的IR2引脚
   outb (PIC_8259A_SLAVE_ICW4_PORT, 0x01);    // ICW4: 8086模式, 正常EOI

   /* 打开主片上IR0,也就是目前只接受时钟产生的中断 */
   //outb (PIC_8259A_MASTER_OCW1_PORT, 0xfe);
    /*时钟与键盘中断*/
   outb (PIC_8259A_MASTER_OCW1_PORT, 0xfc);
   outb (PIC_8259A_SLAVE_OCW1_PORT, 0xff);

}

interrupt_func_handler register_interrupt_func(uint16_t INTERRUPT_NUM, interrupt_func_handler func){
    interrupt_state old = close_interrupt();
    interrupt_func_handler old_handler = IDT_FUNCS[INTERRUPT_NUM];
    IDT_FUNCS[INTERRUPT_NUM] = func;
    set_interrupt_state(old);
    return old_handler; 
}

//默认的中断其实就是打印一句话
static void default_interrupt_func(void){
    uint32_t INTERRUPT_NUM;
    asm volatile(" \
    movl 8(%%ebp),%%eax; \
    movl %%eax, %0; \
    "::"m"(INTERRUPT_NUM):"memory");
    INTERRUPT_NUM &= 0x000000FF;
    printf("this is default interruput func interupt_num:%d\n",INTERRUPT_NUM);
}

//把所有中断注册为默认的
static void register_default_func(void){
    for(int i = 0; i < IDT_SIZE; i++){
        register_interrupt_func(i,default_interrupt_func);
    }
}


void interrupt_init(){
    idt_desc_init();
    register_default_func();
    pic_init();
    /* 加载idt */
   uint64_t idt_operand = ((sizeof(IDT) - 1) | ((uint64_t)(uint32_t)IDT << 16));
   asm volatile("lidt %0" : : "m" (idt_operand));
}



#define EFLAGS_IF 0x200
interrupt_state get_interrupt_state(){
    interrupt_state state;
    uint32_t eflags;
    asm volatile("\
    pushf;\
    movl (%%esp), %%eax;\
    addl 4, %%esp;"\
    :"=a"(eflags)::);
    if(eflags&EFLAGS_IF){
        state = INTERRUPT_ENABLE;
    }
    else{
        state = INTERRUPT_DISABLE; 
    }
    return state;
}


interrupt_state open_interrupt(){
    interrupt_state state = get_interrupt_state();
    if(state != INTERRUPT_ENABLE){
       asm volatile("sti;");
    }
    return state;
}


interrupt_state close_interrupt(){
    interrupt_state state = get_interrupt_state();
    if(state != INTERRUPT_DISABLE){
        asm volatile("cli;");
    }
    return state; 
}

interrupt_state set_interrupt_state(interrupt_state state){
    if(state == INTERRUPT_ENABLE){
        return open_interrupt();
    }
    else if(state == INTERRUPT_DISABLE){
        return close_interrupt();
    }
}


static uint64_t global_tick = 0;
void timer_interrupt(void){
    struct task_struct* pcb = get_current_pcb();
    global_tick++;
    /*
    put_str("\nglobal_tick:");
    put_int(global_tick);
    put_str("\n");
    */
    assert(pcb->stack_magic == STACK_OVERFLOW_MAGIC_NUM);
    pcb->ticks--;
    pcb->elapsed_ticks++;
    if(pcb->ticks <= 0){
        schedule();
    }
}
