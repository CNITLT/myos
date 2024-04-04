#ifndef __QUEUE_H
#define __QUEUE_H
#include "mutex.h"

#define IO_QUEUE_MAX_SIZE 128
struct IO_Queue{
    struct semaphore write_semaphore;
    struct semaphore read_semaphore;
    struct mutex mutex;
    int start;
    int end;
    int size;
    char buff[IO_QUEUE_MAX_SIZE];
};

/*
@brief 初始化IO队列
@param p_io_queue: struct IO_Queue* :IO队列地址
*/
void IO_Queue_init(struct IO_Queue* p_io_queue);


/*
@brief 向IO队列里面插入值
@param p_io_queue: struct IO_Queue* :IO队列地址
@param value: char : 待插入的值
*/
void IO_Queue_push(struct IO_Queue* p_io_queue, char value);


/*
@brief 判断IO队列是否已满 
@param p_io_queue: struct IO_Queue* :IO队列地址
*/
bool IO_Queue_is_full(struct IO_Queue* p_io_queue);


/*
@brief 从IO队列里取出值
@return char 返回值
*/
char IO_Queue_pop(struct IO_Queue* p_io_queue);

/*
@brief 查看IO队列里的第一个值，不弹出
@return char 返回值
*/
char IO_Queue_front(struct IO_Queue* p_io_queue);

/*
@brief 查看IO队列里的最后一个值，不弹出
@return char 返回值
*/
char IO_Queue_back(struct IO_Queue* p_io_queue);


#endif