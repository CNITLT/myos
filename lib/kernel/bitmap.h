#ifndef __BITMAP_H
#define __BITMAP_H
#include "stdint.h"


typedef struct bitmap{
    size_t len_byte;
    uint8_t* bits;
} bitmap;

typedef enum bit_state{BIT_STATE_UNUSE = 0, BIT_STATE_USE} bit_state;
#define BITMAP_RANGE_NOTFOUND -1

/*
@brief 初始化bitmap,将bits后长len_byte内的数据清0
@param p_bitmap:bitmap*:需要清空的bitmap地址
*/
void bitmap_init(bitmap* p_bitmap);


/*
@brief 根据以bit为单位的下标获取bitmap内的数值
@param p_bitmap:bitmap*:bitmap地址
@param index_bit: uint32_t: 以bit为单位的下标索引
@return bit_state bitmap内对应的index_bit下标的值
*/
bit_state bitmap_get(bitmap* p_bitmap, uint32_t index_bit);


/*
@brief 根据以bit为单位的下标设置bitmap内的数值
@param p_bitmap:bitmap*:bitmap地址
@param index_bit: uint32_t: 以bit为单位的下标索引
@param value:uint8_t: 要设置的值
@return bit_state 该位置的旧值
*/
bit_state bitmap_set(bitmap* p_bitmap, uint32_t index_bit, bit_state value);

/*
@brief 查找bitmap内有没有以bit为单位长度的range_bit连续范围的未使用的资源
@param p_bitmap:bitmap*:bitmap地址
@param range_bit: uint32_t: 需要的查找的连续范围的空位范围长度
@return uint32_t 若查找到返回bit为单位的下标，找不到返回BITMAP_RANGE_NOTFOUND
*/
uint32_t bitmap_find_range(bitmap* p_bitmap, uint32_t range_bit);

/*
@brief 设置bitmap1️以index_bit为起点的长range_bit范围的值
@param p_bitmap:bitmap*:bitmap地址
@param index_bit: uint32_t: 以bit为单位的下标索引
@param range_bit: uint32_t: 以bit为单位的范围长度
@param value:bit_state:需要设置的值
*/
void bitmap_range_set(bitmap* p_bitmap, uint32_t index_bit, uint32_t range_bit, bit_state value);
#endif 