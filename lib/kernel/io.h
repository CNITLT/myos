#ifndef __LIB_KERNEL_IO_h
#define __LIB_KERNEL_IO_h
#include "stdint.h"
//以下函数都用内联实现，为了减少栈的消耗，故没有对应的.c文件
//这是AT&T格式的汇编，源操作数与目的操作数的位置与intel格式的反着的

/*
@brief 向某个端口写一个字节数据
@param port:uint16_t, 端口地址
@param data:uint8_t, 写入的1字节数据
*/
static inline void outb(uint16_t port, uint8_t data){
    asm volatile (
        "outb %b0, %w1"::"a"(data), "d"(port)
    );
}

/*
@brief 向某个端口写两个个字节数据
@param port:uint16_t, 端口地址
@param data:uint16_t, 写入的2字节数据
*/
static inline void outw(uint16_t port, uint16_t data){
    asm volatile (
        "outw %w0, %w1"::"a"(data), "d"(port)
    );
}

/*
@brief 从某个端口读一个字节数据
@param port:uint16_t, 端口地址
@return uint8_t 读取的1字节数据
*/
static inline uint8_t inb(uint16_t port){
    uint8_t data;
    asm volatile (
        "inb %w1, %b0":"=a"(data):"d"(port)
    );
    return data;
}

/*
@brief 从某个端口读一个字节数据
@param port:uint16_t, 端口地址
@return uint16_t 读取的2字节数据
*/
static inline uint16_t inw(uint16_t port){
    uint16_t data;
    asm volatile (
        "inw %w1, %w0":"=a"(data):"d"(port)
    );
    return data;
}

#endif