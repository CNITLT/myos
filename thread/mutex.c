#include "mutex.h"
#include "debug.h"
#include "thread.h"
void semaphore_init(struct semaphore* p_semaphore, uint32_t value){
    p_semaphore->value = value;
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

    p_semaphore->value++;  

    while(!list_empty(&p_semaphore->waiters)){
        //debug("semaphore_add len waiters:%d\n", list_len(&p_semaphore->waiters));
        struct list_node * node = list_pop_front(&p_semaphore->waiters);
        struct task_struct* pcb = elem2entry(struct task_struct, general_tag, node);
        //debug("semaphore_add before unblock len waiters:%d\n", list_len(&p_semaphore->waiters));
        thread_unblock(pcb);
    }
    set_interrupt_state(old_state);
}


void semaphore_sub(struct semaphore* p_semaphore){
    //warning:: 多核下关中断不适用
    //hack:: 若boches改多核这里需要修改 
    interrupt_state old_state = close_interrupt();
    struct task_struct* pcb = get_current_pcb();
    int count = 0;
    while(p_semaphore->value <= 0){
        assert(!find_node(&p_semaphore->waiters,&pcb->general_tag));
        list_push_back(&p_semaphore->waiters, &pcb->general_tag);
        //debug("semaphore_sub pcb->name:%s semaphore addr:%x count:%d\n", pcb->name, p_semaphore, ++count);
        thread_block(TASK_BLOCKED);
        //debug("semaphore_sub after thread_block p_semaphore->value:%d\n",p_semaphore->value);
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