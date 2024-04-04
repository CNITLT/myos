#include "queue.h"


void IO_Queue_init(struct IO_Queue* p_io_queue){
    semaphore_init(&p_io_queue->write_semaphore, IO_QUEUE_MAX_SIZE);
    semaphore_init(&p_io_queue->read_semaphore, IO_QUEUE_MAX_SIZE);
    mutex_init(&p_io_queue->mutex);
    p_io_queue->read_semaphore.value = 0;
    p_io_queue->start = 0;
    p_io_queue->end = 0;
    p_io_queue->size = 0;
}

void IO_Queue_push(struct IO_Queue* p_io_queue, char value){
    semaphore_sub(&p_io_queue->write_semaphore);
    lock(&p_io_queue->mutex);
    p_io_queue->buff[p_io_queue->end] = value;
    p_io_queue->end = (p_io_queue->end+1) % IO_QUEUE_MAX_SIZE;
    p_io_queue->size++;//这个size没必要判断是否超出，用信号量来替代了
    unlock(&p_io_queue->mutex); 
    semaphore_add(&p_io_queue->read_semaphore);
}


char IO_Queue_pop(struct IO_Queue* p_io_queue){
    semaphore_sub(&p_io_queue->read_semaphore);
    lock(&p_io_queue->mutex); 
    int value = p_io_queue->buff[p_io_queue->start];
    p_io_queue->start = (p_io_queue->start+1) % IO_QUEUE_MAX_SIZE;
    p_io_queue->size--;
    unlock(&p_io_queue->mutex);
    semaphore_add(&p_io_queue->write_semaphore);
    return value;
}


char IO_Queue_front(struct IO_Queue* p_io_queue){
    semaphore_sub(&p_io_queue->read_semaphore);
    lock(&p_io_queue->mutex); 
    int value = p_io_queue->buff[p_io_queue->start];
    unlock(&p_io_queue->mutex);
    semaphore_add(&p_io_queue->read_semaphore);
    return value;
}


char IO_Queue_back(struct IO_Queue* p_io_queue){
    semaphore_sub(&p_io_queue->read_semaphore);
    lock(&p_io_queue->mutex); 
    int value = p_io_queue->buff[p_io_queue->end-1];
    unlock(&p_io_queue->mutex);
    semaphore_add(&p_io_queue->read_semaphore);
    return value;
}


bool IO_Queue_is_full(struct IO_Queue* p_io_queue){
    lock(&p_io_queue->mutex); 
    int size = p_io_queue->size;
    unlock(&p_io_queue->mutex); 
    return size == IO_QUEUE_MAX_SIZE;
}

