#include "mutex.h"
#include "debug.h"
#include "thread.h"
void semaphore_init(struct semaphore* p_semaphore, uint32_t value){
    p_semaphore->value = value;
    p_semaphore->max_value = value;
    list_init(&p_semaphore->waiters);
}


void mutex_init(struct mutex* p_mutex){
    p_mutex->holder = NULL;
    semaphore_init(&p_mutex->semaphore, 1);
    p_mutex->holder_repeat_lock_num = 0;
}

void semaphore_add(struct semaphore* p_semaphore){
    //warning:: 多核下关中断不适用
    //hack:: 若boches改多核这里需要修改
    interrupt_state old_state = close_interrupt();

    if(p_semaphore->value < p_semaphore->max_value){
        p_semaphore->value++;  
        assert(p_semaphore->value <= p_semaphore->max_value);
    }
    while(!list_empty(&p_semaphore->waiters)){
        struct list_node * node = list_pop_front(&p_semaphore->waiters);
        struct task_struct* pcb = elem2entry(struct task_struct, general_tag, node);
        thread_unblock(pcb);
    }
    set_interrupt_state(old_state);
}


void semaphore_sub(struct semaphore* p_semaphore){
    //warning:: 多核下关中断不适用
    //hack:: 若boches改多核这里需要修改 
    interrupt_state old_state = close_interrupt();
    struct task_struct* pcb = get_current_pcb();
    while(p_semaphore->value <= 0){
        assert(!find_node(&p_semaphore->waiters,&pcb->general_tag));
        list_push_back(&p_semaphore->waiters, &pcb->general_tag);
        thread_block(TASK_BLOCKED);
    }
    p_semaphore->value--;
    assert(p_semaphore->value>=0);
    set_interrupt_state(old_state);
}



void lock(struct mutex* p_mutex){
    struct task_struct* pcb = get_current_pcb();
    if(p_mutex->holder != pcb){
        semaphore_sub(&p_mutex->semaphore);
        p_mutex->holder = pcb;
    }
    else{
        p_mutex->holder_repeat_lock_num++;
    }
}


void unlock(struct mutex* p_mutex){
    struct task_struct* pcb = get_current_pcb();
    assert(p_mutex->holder == pcb);
    if(p_mutex->holder_repeat_lock_num > 0){
        p_mutex->holder_repeat_lock_num--;
        return;
    }
    else{
        assert(p_mutex->holder_repeat_lock_num == 0);
        p_mutex->holder = NULL;
        semaphore_add(&p_mutex->semaphore);
    }
}