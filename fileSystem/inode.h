#ifndef __FILESYSTEM_INODE_H
#define __FILESYSTEM_INODE_H

#include "stdint.h"
#include "list.h"
#include "ide.h"
#include "fs.h"

#define I_NODE_SECTOR_SIZE 16
// 直接块数量
#define I_NODE_LAYER0_BLCOK_SIZE 12
#define I_NODE_LAYER1_BLOCK_SIZE 4
// 一级块内的直接块数量
#define I_NODE_LAYER0_SIZE_PER_LAYER1 (BLOCK_SIZE / sizeof(uint32_t))
#define MAX_FILE_CONTENT_SIZE ((I_NODE_LAYER0_BLCOK_SIZE + I_NODE_LAYER1_BLOCK_SIZE * I_NODE_LAYER0_SIZE_PER_LAYER1) * BLOCK_SIZE)
struct Inode
{
    uint32_t i_no; // inode编号
    uint32_t i_size; // 文件大小 （书上版本代表 有效目录项*目录项大小， 魔改后对目录来说仅代表(历史最大值) * 目录项大小，也就是说对目录来说只会往大了分配但不删除）
    uint32_t i_open_cnts; // 文件打开次数
    bool write_deny; // 写文件前检测此标识，等价于一个锁
    uint32_t i_sectors[I_NODE_SECTOR_SIZE]; // 数据块索引，仅支持到1级索引
    struct list_node inode_tag; // 用于内存里打开的inode列表节点
};

struct Inode_position {
    bool is_in_two_section; // 判断inode本身的信息是否跨区
    uint32_t section_lba; // 在那块section上
    uint32_t offset_in_section; // inode信息起点在扇区上的偏移
};
/*
    @brief 获取inode所在扇区的位置信息
    @param p_part : struct Partition * : 扇区信息
    @param inode_no : uint32_t:  inode编号
    @param p_inode_position: struct Inode_position *: 返回的信息
*/
void inode_locate(struct Partition *p_part, uint32_t inode_no, struct Inode_position *p_inode_pos);

/*
    @brief 写入inode到分区
    @param p_part : struct Partition * : 扇区信息
    @param inode : struct Inode *: inode信息
    @param io_buff : void *: 主调函数提供的缓冲区，如果要给则至少需要2个扇区大小, 或者给null由函数内部处理
*/
void inode_sync(struct Partition *p_part, struct Inode *p_inode, void *io_buff);

/**
 *  @brief 从内存中已经打开的inode列表里找到有无对应的inode信息
    @param p_part : struct Partition * : 扇区信息
    @param inode_no : uint32_t:  inode编号
    @return struct Inode * 打开的inode信息地址
*/
struct Inode* find_opened_inode(struct Partition *p_part, uint32_t inode_no);

/*
    @brief 打开inode,即加载inode信息到内存
    @param p_part : struct Partition * : 分区信息
    @param inode_no : uint32_t:  inode编号
    @return struct Inode * 打开的inode信息地址
*/
struct Inode* inode_open(struct Partition *p_part, uint32_t inode_no);

/*
    @brief 关闭inode
    @param p_inode : struct Inode * : inode信息
*/
void inode_close(struct Inode* p_inode);


/*
    @brief 回收inode_no所代表的资源
    @param p_part : struct Partition * : 分区信息
    @param inode_no : uint32_t:  inode编号
    @return 成功返回true 否则false
*/
bool inode_release(struct Partition *p_part, uint32_t inode_no);

/*
    @brief 初始化inode
    @param p_inode : struct Inode * : inode信息
    @param inode_no : uint32_t : inode编号
*/
void inode_init(struct Inode* p_inode, uint32_t inode_no);

/*
    @brief 获取inode在磁盘所有block的lba地址汇总
    @param p_part: struct Partition *: 分区信息
    @param p_inode:  struct Inode * :搜寻的inode节点
    @param p_all_block_lba_ret: uint32_t **: 返回值，存储函数分配的所有block lba地址的数组地址的指针地址, 使用完后由外部释放
    @param p_all_block_lba_count_ret: uint32_t *: 返回值，all_block_lba数组元素个数
*/
void get_inode_all_block_lba(struct Partition* p_part, struct Inode *p_inode, uint32_t **p_all_block_lba_ret, uint32_t *p_all_block_lba_count_ret);

/*
   @brief 给一个inode分配一个block, 分配成功会同步到p_all_block_lba内，及inode，磁盘等相关信息
   @param p_part: struct Partition *: 分区信息
   @param p_inode:  struct Inode * :搜寻的inode节点
   @param p_all_block_lba_ret: uint32_t *: 存储函数分配的所有block lba地址的数组地址的指针地址
   @param all_block_lba_count_ret: uint32_t : all_block_lba数组元素个数
   @param all_block_index: int32_t : 在所有块内的索引
   @return int32_t: 成果返回新分配的直接块地址, 否则为-1
*/
int32_t alloc_inode_all_block(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t all_block_lba_count, int32_t all_block_index);

/*
   @brief 释放inode的一个block, 释放成功会同步到p_all_block_lba内，及inode，磁盘等相关信息, 对于1级块，若1级块内无直接块则对应释放1级块
   @param p_part: struct Partition *: 分区信息
   @param p_inode:  struct Inode * :搜寻的inode节点
   @param p_all_block_lba_ret: uint32_t *: 存储函数分配的所有block lba地址的数组地址的指针地址
   @param all_block_lba_count_ret: uint32_t : all_block_lba数组元素个数
   @param all_block_index: int32_t : 在所有块内的索引
   @return int32_t: 释放成功返回0， 否则-1， 若本身给出的块不存在，同时视为释放成功
*/
int32_t free_inode_all_block(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t all_block_lba_count, int32_t all_block_index);

/*
   @brief 从特定的inode内读取count数据量到data内
   @param p_part: struct Partition *: 分区信息
   @param p_inode:  struct Inode * :操作的inode节点
   @param p_all_block_lba_ret: uint32_t *: 存储函数分配的所有block lba地址的数组地址的指针地址
   @param p_all_block_lba_count_ret: uint32_t : all_block_lba数组元素个数
   @param all_block_index: int32_t : 在所有块内的索引
   @param pos: int32_t :读取的起点，字节为单位, 超出返回会导致失败
   @param data: void *: 读取数据存入缓冲区
   @param count: size_t :要读取的字节数
   @return int32_t: 成功读取的字节数，读取失败返回-1
*/
int32_t read_data_from_inode(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t all_block_lba_count, int32_t pos, void *data, size_t count);

/*
   @brief 向特定的inode内写入data里的count数据量
   @param p_part: struct Partition *: 分区信息
   @param p_inode:  struct Inode * :操作的inode节点
   @param p_all_block_lba_ret: uint32_t *: 存储函数分配的所有block lba地址的数组地址的指针地址
   @param all_block_lba_count_ret: uint32_t : all_block_lba数组元素个数
   @param all_block_index: int32_t : 在所有块内的索引
   @param pos: int32_t :写入的起点，字节为单位, 起点仅能在文件大小内或者文件末尾
   @param data: void *: 写入数据的缓冲区
   @param count: size_t :要写入的字节数
   @return int32_t: 成功写入的字节数，读取失败返回-1
*/
int32_t write_data_to_inode(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t all_block_lba_count, int32_t pos, void *data, size_t count);

#endif