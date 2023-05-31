#include "interrupt.h"
#include "print.h"
#define ERROR_CODE_RET asm volatile("addl $4,%esp;iret;");
#define DEFAULT_RET asm volatile("iret;");
#define NAKEDFUNC __attribute__((naked))
//代理中断函数，主要用来处理返回到正确的地方,普通函数都是ret返回
#define IDT_FUNC_ENTRY_PROXY(INTERRUPT_NUM, RET_TYPE) \
NAKEDFUNC static void  IDT_FUNC_ENTRY_PROXY##INTERRUPT_NUM(void){ \
    asm volatile (" \
    push %ds; \
    push %es; \
    push %fs; \
    push %gs; \
    pushal;"); \
    asm volatile(" \
    call *%0; \
    "::"m"(IDT_FUNCS[INTERRUPT_NUM])); \
    asm volatile(" \
    popal; \
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
   IDT_FUNC_ENTRY_PROXY31
}; //代理函数的地址

static const interrupt_gate_desc default_value = DEFAULT_INTERRUPT_GATE_DESC_VALUE;
void idt_desc_init(void){
    for(int i = 0; i < IDT_SIZE; i++){
        IDT[i] = default_value;
        IDT[i].func_offset_low = (uint32_t)IDT_ENTRY_PROXYS[i] & 0x0000FFFF;
        IDT[i].func_offset_high = ((uint32_t)IDT_ENTRY_PROXYS[i] & 0xFFFF0000) >> 16; 
    }
}

interrupt_func_handler register_interrupt_func(uint16_t INTERRUPT_NUM, interrupt_func_handler func){
    interrupt_func_handler old_handler = IDT_FUNCS[INTERRUPT_NUM];
    IDT_FUNCS[INTERRUPT_NUM] = func;
    return old_handler; 
}

//默认的中断其实就是打印一句话
static void default_interrupt_func(void){
    put_str("this is default interruput func.\n");
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

    /* 加载idt */
   uint64_t idt_operand = ((sizeof(IDT) - 1) | ((uint64_t)(uint32_t)IDT << 16));
   asm volatile("lidt %0" : : "m" (idt_operand));
}