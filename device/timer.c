#include "timer.h"
#include "io.h"
#include "thread.h"
#include "debug.h"
#define TIMER_CONTROL_PORT 0x43
#define TIMER_CONTROL_SELECT_TIMER0 (0x0 << 6) //选择计数器0
#define TIMER_CONTROL_SELECT_TIMER1 (0x1 << 6) //选择计数器1
#define TIMER_CONTROL_SELECT_TIMER2 (0x2 << 6) //选择计数器2

#define TIMER_CONTROL_RW_LOCK (0x0 << 4) //锁存数据供CPU读
#define TIMER_CONTROL_RW_LOW (0x1 << 4) //读写低8位
#define TIMER_CONTROL_RW_HIGH (0x2 << 4) //读写高8位
#define TIMER_CONTROL_RW_LOW_HIGH (0x3 << 4) //先读写低8位再读写高8位

#define TIMER_CONTROL_MODE0 (0x0 << 1) //计数结束中断方式
#define TIMER_CONTROL_MODE1 (0x1 << 1) //硬件可重处罚单稳方式
#define TIMER_CONTROL_MODE2 (0x2 << 1) //比率发生器
#define TIMER_CONTROL_MODE3 (0x3 << 1) //方便发生器
#define TIMER_CONTROL_MODE4 (0x4 << 1) //软件触发选通
#define TIMER_CONTROL_MODE5 (0x5 << 1) //硬件触发选通

#define TIMER_CONTROL_BCD 0x1 //用BCD模式读写
#define TIMER_CONTROL_BINARY 0x0 //用二进制模式读写

#define TIMER0_PORT 0x40
#define TIMER1_PORT 0x41
#define TIMER2_PORT 0x42

#define CLK_FREQUENCY 1193180  //clk引脚接的晶振频率
#define TIMER_INTR_FREQUENCY 100 //设置的时钟中断频率，单位HZ,即10ms一次中断

volatile size_t g_tick = 0;

void timer_init(){
    outb(TIMER_CONTROL_PORT, TIMER_CONTROL_SELECT_TIMER0 | TIMER_CONTROL_RW_LOW_HIGH | TIMER_CONTROL_MODE2 | TIMER_CONTROL_BINARY);
    outb(TIMER0_PORT, (uint16_t)(CLK_FREQUENCY / TIMER_INTR_FREQUENCY) &0xFF);
    outb(TIMER0_PORT, ((uint16_t)(CLK_FREQUENCY / TIMER_INTR_FREQUENCY) &0xFF00)>>8);
    //注册时钟中断函数
    register_interrupt_func(0x20, timer_interrupt); 
}

size_t ms_per_tick(){
    return 1000 / TIMER_INTR_FREQUENCY;
}

void sleep_ticks(size_t sleep_time_tick){
    size_t start_tick = g_tick;
    while((g_tick - start_tick) < sleep_time_tick){
        //debug("start_tick:%d sleep_time_tick:%d g_tick:%d intr_status:%d\n", start_tick, sleep_time_tick,g_tick,get_interrupt_state());
        sys_thread_yield();
    }
}

void sleep_ms(size_t sleep_time_ms){
    size_t sleep_time_tick = DIV_ROUND_UP(sleep_time_ms, ms_per_tick());
    sleep_ticks(sleep_time_tick);
}