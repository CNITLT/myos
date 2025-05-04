#ifndef __DEVICE_TIMER_H
#define __DEVICE_TIMER_H
#include "stddef.h"
#include "stdint.h"
#include "stdarg.h"
extern volatile size_t g_tick;
/*
*初始化时钟计数器, 每隔一定时间发送一次中断
*/
void timer_init();

/*
@brief 单个时钟中断对应的ms时间
*/
size_t ms_per_tick();


/*
@brief 以时钟滴答为单位的睡眠时间
@param sleep_time_tick: size_t :以时钟滴答为单位的睡眠时间
*/
void sleep_ticks(size_t sleep_time_tick);


/*
@brief 以毫秒为单位的睡眠
@param sleep_time_ms: size_t :以毫秒为单位的睡眠时间
*/
void sleep_ms(size_t sleep_time_ms);
#endif