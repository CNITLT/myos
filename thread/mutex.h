#ifndef __MUTEX_H
#define __MUTEX_H
#include "list.h"
#include "interrupt.h"
#include "stddef.h"
#include "stdint.h"
#include "thread.h"

//信号量
struct semaphore{
    uint32_t value;
    struct list waiters;
    uint32_t max_value;
};


struct mutex
{
    struct task_struct* holder;//持有者线程的PCB
    struct semaphore semaphore;//信号量
    uint32_t holder_repeat_lock_num;//持有者重复上锁的次数
};


/*
@brief 初始化信号量
@param p_semaphore: struct semaphore*  :信号量地址
@param value: uint32_t : 信号量初值,也即资源数
*/
void semaphore_init(struct semaphore* p_semaphore, uint32_t value);


/*
@brief 初始化互斥量
@param p_mutex: struct mutex*  :互斥量地址
*/
void mutex_init(struct mutex* p_mutex);


/*
@brief 信号量的v操作
@param p_semaphore: struct semaphore*  :信号量地址
*/
void semaphore_add(struct semaphore* p_semaphore);



/*
@brief 信号量的p操作, 若无资源则阻塞
@param p_semaphore: struct semaphore*  :信号量地址
*/
void semaphore_sub(struct semaphore* p_semaphore);

/*
@brief 对互斥量加锁
@param p_mutex: struct mutex*  :互斥量地址
*/
void lock(struct mutex* p_mutex);


/*
@brief 对互斥量解锁
@param p_mutex: struct mutex*  :互斥量地址
*/
void unlock(struct mutex* p_mutex);

#endif