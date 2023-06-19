#ifndef __KERNEL_E820_H
#define __KERNEL_E820_H
#include "stdint.h"

//可以被操作系统使用
#define e820_type_memory 1 
//被保留的，不允许被操作系统使用
#define e820_type_reserved 2 
//其他值是保留，留着以后扩展
typedef struct e820_entry{
    uint64_t addr; //基地址
    uint64_t length; //长度
    uint32_t type; //类型
} e820_entry;


/*
@brief 打印E820信息表
*/
void print_e820_table();

/*
@brief 获取E820地址表长度
@return size_t 表长度
*/
size_t get_e820_length();


/*
@brief 获取e820表首地址
@return const e820_entry*
*/
const e820_entry* get_e820_table();


/*
@brief 获取最大可用的物理内存空间
@param p_e820_entry:e820_entry*: 写入的返回值地址
*/
void get_max_memory_e820_entry(e820_entry* p_e820_entry);
#endif 