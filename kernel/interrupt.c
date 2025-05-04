#include "interrupt.h"
#include "print.h"
#include "io.h"
#include "stddef.h"
#include "thread.h"
#include "debug.h"
#include "init.h"
#include "syscall_init.h"
#include "timer.h"
//控制默认中断函数是否打印DEBUG信息
#define DEFAULT_INTR_FUNC_PF


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
#define IDT_SIZE 0x81


//#define ERROR_CODE_RET asm volatile("addl $4,%esp;iret;");
//#define DEFAULT_RET asm volatile("iret;");
//统一一下中断返回的方法，没中断错误码的会push一个伪造的0中断错误码进去，这样中断返回就能统一方法
#define INTR_RET asm volatile("addl $4,%esp;iret;");
#define INTR_PUSH_FAKE_ERROR_CODE asm volatile("push $0;");
#define INTR_NONE_PUSH

void intr_exit_from(struct interrupt_stack* p_intr_stack){
    asm volatile("movl %0, %%esp"::"g"(p_intr_stack):"memory");
    //与IDT_FUNC_ENTRY_PROXY里面的出栈顺序一直，不过不能忘了要addl $4, %esp;把中断号出站
    asm volatile(" \
        addl $4, %esp; \
        popa; \
        pop %gs; \
        pop %fs; \
        pop %es; \
        pop %ds;"); \
    INTR_RET;
}

//代理中断函数，主要用来处理返回到正确的地方,普通函数都是ret返回
#define IDT_FUNC_ENTRY_PROXY(INTERRUPT_NUM, PUSH_TYPE) \
NAKEDFUNC static void  IDT_FUNC_ENTRY_PROXY##INTERRUPT_NUM(void){ \
    PUSH_TYPE \
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
    INTR_RET \
} 


static interrupt_func_handler IDT_FUNCS[IDT_SIZE];//实际的处理函数


interrupt_gate_desc IDT[IDT_SIZE];
IDT_FUNC_ENTRY_PROXY(0,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(1,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(2,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(3,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(4,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(5,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(6,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(7,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(8,INTR_NONE_PUSH)
IDT_FUNC_ENTRY_PROXY(9,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(10,INTR_NONE_PUSH)
IDT_FUNC_ENTRY_PROXY(11,INTR_NONE_PUSH)
IDT_FUNC_ENTRY_PROXY(12,INTR_NONE_PUSH)
IDT_FUNC_ENTRY_PROXY(13,INTR_NONE_PUSH)
IDT_FUNC_ENTRY_PROXY(14,INTR_NONE_PUSH)
IDT_FUNC_ENTRY_PROXY(15,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(16,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(17,INTR_NONE_PUSH)
IDT_FUNC_ENTRY_PROXY(18,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(19,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(20,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(21,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(22,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(23,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(24,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(25,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(26,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(27,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(28,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(29,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(30,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(31,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(32,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(33,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(34,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(35,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(36,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(37,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(38,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(39,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(40,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(41,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(42,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(43,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(44,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(45,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(46,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(47,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(48,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(49,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(50,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(51,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(52,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(53,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(54,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(55,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(56,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(57,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(58,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(59,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(60,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(61,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(62,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(63,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(64,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(65,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(66,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(67,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(68,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(69,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(70,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(71,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(72,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(73,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(74,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(75,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(76,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(77,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(78,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(79,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(80,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(81,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(82,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(83,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(84,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(85,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(86,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(87,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(88,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(89,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(90,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(91,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(92,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(93,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(94,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(95,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(96,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(97,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(98,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(99,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(100,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(101,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(102,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(103,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(104,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(105,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(106,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(107,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(108,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(109,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(110,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(111,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(112,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(113,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(114,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(115,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(116,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(117,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(118,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(119,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(120,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(121,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(122,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(123,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(124,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(125,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(126,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(127,INTR_PUSH_FAKE_ERROR_CODE)
IDT_FUNC_ENTRY_PROXY(128,INTR_PUSH_FAKE_ERROR_CODE)


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
   IDT_FUNC_ENTRY_PROXY47,
   IDT_FUNC_ENTRY_PROXY48,
   IDT_FUNC_ENTRY_PROXY49,
   IDT_FUNC_ENTRY_PROXY50,
   IDT_FUNC_ENTRY_PROXY51,
   IDT_FUNC_ENTRY_PROXY52,
   IDT_FUNC_ENTRY_PROXY53,
   IDT_FUNC_ENTRY_PROXY54,
   IDT_FUNC_ENTRY_PROXY55,
   IDT_FUNC_ENTRY_PROXY56,
   IDT_FUNC_ENTRY_PROXY57,
   IDT_FUNC_ENTRY_PROXY58,
   IDT_FUNC_ENTRY_PROXY59,
   IDT_FUNC_ENTRY_PROXY60,
   IDT_FUNC_ENTRY_PROXY61,
   IDT_FUNC_ENTRY_PROXY62,
   IDT_FUNC_ENTRY_PROXY63,
   IDT_FUNC_ENTRY_PROXY64,
   IDT_FUNC_ENTRY_PROXY65,
   IDT_FUNC_ENTRY_PROXY66,
   IDT_FUNC_ENTRY_PROXY67,
   IDT_FUNC_ENTRY_PROXY68,
   IDT_FUNC_ENTRY_PROXY69,
   IDT_FUNC_ENTRY_PROXY70,
   IDT_FUNC_ENTRY_PROXY71,
   IDT_FUNC_ENTRY_PROXY72,
   IDT_FUNC_ENTRY_PROXY73,
   IDT_FUNC_ENTRY_PROXY74,
   IDT_FUNC_ENTRY_PROXY75,
   IDT_FUNC_ENTRY_PROXY76,
   IDT_FUNC_ENTRY_PROXY77,
   IDT_FUNC_ENTRY_PROXY78,
   IDT_FUNC_ENTRY_PROXY79,
   IDT_FUNC_ENTRY_PROXY80,
   IDT_FUNC_ENTRY_PROXY81,
   IDT_FUNC_ENTRY_PROXY82,
   IDT_FUNC_ENTRY_PROXY83,
   IDT_FUNC_ENTRY_PROXY84,
   IDT_FUNC_ENTRY_PROXY85,
   IDT_FUNC_ENTRY_PROXY86,
   IDT_FUNC_ENTRY_PROXY87,
   IDT_FUNC_ENTRY_PROXY88,
   IDT_FUNC_ENTRY_PROXY89,
   IDT_FUNC_ENTRY_PROXY90,
   IDT_FUNC_ENTRY_PROXY91,
   IDT_FUNC_ENTRY_PROXY92,
   IDT_FUNC_ENTRY_PROXY93,
   IDT_FUNC_ENTRY_PROXY94,
   IDT_FUNC_ENTRY_PROXY95,
   IDT_FUNC_ENTRY_PROXY96,
   IDT_FUNC_ENTRY_PROXY97,
   IDT_FUNC_ENTRY_PROXY98,
   IDT_FUNC_ENTRY_PROXY99,
   IDT_FUNC_ENTRY_PROXY100,
   IDT_FUNC_ENTRY_PROXY101,
   IDT_FUNC_ENTRY_PROXY102,
   IDT_FUNC_ENTRY_PROXY103,
   IDT_FUNC_ENTRY_PROXY104,
   IDT_FUNC_ENTRY_PROXY105,
   IDT_FUNC_ENTRY_PROXY106,
   IDT_FUNC_ENTRY_PROXY107,
   IDT_FUNC_ENTRY_PROXY108,
   IDT_FUNC_ENTRY_PROXY109,
   IDT_FUNC_ENTRY_PROXY110,
   IDT_FUNC_ENTRY_PROXY111,
   IDT_FUNC_ENTRY_PROXY112,
   IDT_FUNC_ENTRY_PROXY113,
   IDT_FUNC_ENTRY_PROXY114,
   IDT_FUNC_ENTRY_PROXY115,
   IDT_FUNC_ENTRY_PROXY116,
   IDT_FUNC_ENTRY_PROXY117,
   IDT_FUNC_ENTRY_PROXY118,
   IDT_FUNC_ENTRY_PROXY119,
   IDT_FUNC_ENTRY_PROXY120,
   IDT_FUNC_ENTRY_PROXY121,
   IDT_FUNC_ENTRY_PROXY122,
   IDT_FUNC_ENTRY_PROXY123,
   IDT_FUNC_ENTRY_PROXY124,
   IDT_FUNC_ENTRY_PROXY125,
   IDT_FUNC_ENTRY_PROXY126,
   IDT_FUNC_ENTRY_PROXY127,
   IDT_FUNC_ENTRY_PROXY128,
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
    //int80H是系统调用的中断，要给用户态访问，所以是3
    IDT[0x80].attribute.DPL = 3;
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


    // 向OCW1写入，位为1就是屏蔽对应的中断, 位为0是放开对应的中断
    /* 打开主片上IR0,也就是目前只接受时钟产生的中断 */
    //outb (PIC_8259A_MASTER_OCW1_PORT, 0xfe);
    
    /*时钟与键盘中断*/
    //outb (PIC_8259A_MASTER_OCW1_PORT, 0xfc);
    //outb (PIC_8259A_SLAVE_OCW1_PORT, 0xff);

    /*时钟、键盘、硬盘中断*/
    // 放开IPQ2的中断, 这个是级联8259A的中断连接位置
    // f8包含了时钟、键盘、硬盘中断
    outb (PIC_8259A_MASTER_OCW1_PORT, 0xf8);

    // 从片的IRQ14控制硬盘的中断，放开
    // 因为从主片开始编码，所以是14，本质上是从片的IRQ6， 从0编码，实质是第7位
    outb(PIC_8259A_SLAVE_OCW1_PORT, 0xbf);
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
    struct interrupt_stack* p_intr_stack = NULL;
    asm volatile("lea 8(%%ebp), %0":"=g"(p_intr_stack)::"memory");
    #ifdef DEFAULT_INTR_FUNC_PF
    printf("default interruput func interupt_num:%d error_code:0X%x CS:EIP:0x%x:0X%x\n",
        p_intr_stack->interrupt_num, p_intr_stack->err_code,p_intr_stack->cs,p_intr_stack->eip);
    #endif
}

//把所有中断注册为默认的
static void register_default_func(void){
    for(int i = 0; i < IDT_SIZE; i++){
        register_interrupt_func(i,default_interrupt_func);
    }
}

static void int80H_interrupt_func(void){
    syscall_ret_type ret;
    struct interrupt_stack* p_intr_stack = NULL;
    asm volatile("lea 8(%%ebp), %0":"=g"(p_intr_stack)::"memory");
    uint32_t syscallNum = p_intr_stack->eax;
    syscall_param_type arg1 = p_intr_stack->ebx;
    syscall_param_type arg2 = p_intr_stack->ecx;
    syscall_param_type arg3 = p_intr_stack->edx;
    syscall_call_proxy syscall_func = syscall_table[syscallNum];
    if(syscall_func){
        ret = syscall_func(arg1,arg2,arg3);//最多支持3个额外参数 
        p_intr_stack->eax = ret; //返回值设置
    }
}

//注册int80h 系统调用中断
static void register_int80H(void){
    register_interrupt_func(0x80,int80H_interrupt_func);
}

void interrupt_init(){
    idt_desc_init();
    register_default_func();
    register_int80H();
    pic_init();
    /* 加载idt */
   uint64_t idt_operand = ((sizeof(IDT) - 1) | ((uint64_t)(uint32_t)IDT << 16));
   asm volatile("lidt %0" : : "m" (idt_operand));
}



#define EFLAGS_IF 0x200
#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl; popl %0" : "=g" (EFLAG_VAR))

interrupt_state get_interrupt_state(){
    interrupt_state state;
    uint32_t eflags;
    GET_EFLAGS(eflags);
    /*
    //这种写法在开中断的时候会有问题，不知道是哪导致的, 神奇
    asm volatile("\
    pushf;\
    movl (%%esp), %%eax;\
    addl 4, %%esp;"\
    :"=a"(eflags)::);
    */
   /*
   //测了下主要就是addl 4, %%esp这句话导致的，换成pop就没事, 不理解
   asm volatile("\
    pushf;\
    movl (%%esp), %%eax;\
    pop %%ebx;"\ 
    :"=a"(eflags)::);
    */
    
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


void timer_interrupt(void){
    struct task_struct* pcb = get_current_pcb();
    g_tick++;
    //debug("global tick:%d\n", g_tick);
    assert(pcb->stack_magic == STACK_OVERFLOW_MAGIC_NUM);
    pcb->ticks--;
    pcb->elapsed_ticks++;
    //debug("pcb:%x name:%s pcb->ticks:%d pcb->elapsed_ticks:%d\n",pcb, pcb->name, pcb->ticks,pcb->elapsed_ticks);
    if(pcb->ticks <= 0){
        //debug("timer_interrupt schedule \n");
        schedule();
    }
}

