#ifndef __LIB_KERNEL_STRING_H
#define __LIB_KERNEL_STRING_H
#include "stdint.h"
/*
@brief 为某个地址开始的count字节大小的内存区域赋值
@param dst:void* 起始地址
@param value:uint8_t 将要被赋的值
@param count:size_t 数据数量
*/
void memset(void *dst, uint8_t value, size_t count);


/*
@brief 将 src 为起始的内存区域的地址赋值为 dst 为起始开始的内存区域
@param dst:void* 目的起始地址
@param src:const void* 源数据起始地址
@param count:size_t 数据数量
*/
void memcpy(void *dst,const void* src, size_t count);



/*
@brief 逐字节比较 a 为起始地址的内存块和 b 为起始地址的内存块，以首个不相等的内存为最后的结果, 若a<b则返回-1， 若a==b则返回0， 若a>b返回1
@param a:const void* 起始地址a
@param b:const void* 起始地址b
@param count:size_t 数据数量
@return 若a<b则返回-1， 若a==b则返回0， 若a>b返回1
*/
int memcmp(const void* a,const void *b, size_t count);


/*
@brief 计算字符串长度
@param str:const char * 字符串起始地址
@return size_t 字符串长度
*/
size_t strlen(const char *str);

/*
@brief 将字符串src复制到字符串dst
@param dst:char *: 目的数据地址
@param src:char *: 源字符串地址
@return 目的字符串的起始地址，即dst
*/
char* strcpy(char* dst,const char * src);

/*
@brief 比较字符串a和b,以首个不相等的内存为最后的结果, 若a<b则返回-1， 若a==b则返回0， 若a>b返回1
@param a:char* 起始地址a
@param b:char* 起始地址b
@return 若a<b则返回-1， 若a==b则返回0， 若a>b返回1 
*/
int strcmp(const char *a,const char *b);


/*
@brief 从左到右在字符串str内搜索ch首次出现的位置
@param str:const char*: 待被搜索的字符串
@param ch:char 要搜索的字符
@return 如果找到返回指向字符的指针，如果没找到返回NULL
*/
char * strchr(const char* str, char ch);

/*
@brief 从右到左在字符串str内搜索ch首次出现的位置
@param str:const char*: 待被搜索的字符串
@param ch:char 要搜索的字符
@return 如果找到返回指向字符的指针，如果没找到返回NULL
*/
char* strrchr(const char* str, char ch);

/*
@brief 将字符串src拼接到dst后
@param dst:char*: 拼接的目的字符串
@param src:char*: 拼接的源字符串
@return char* 返回拼接后的字符串首地址
*/
char *strcat(char *dst, const char*src);


/*
@brief 统计字符串中ch出现的次数
@param str:const char*: 字符串
@param ch:char: 统计的字符
@return size_t 返回出现的次数
*/
size_t strchrs(const char* str, char ch);


#endif