#ifndef __FILESYSTEM_INODE_H
#define __FILESYSTEM_INODE_H

#include "stdint.h"
#include "list.h"
#include "ide.h"
#include "fs.h"

#define I_NODE_SECTOR_SIZE 13
// 直接块数量
#define I_NODE_LAYER0_BLCOK_SIZE 12
#define I_NODE_LAYER1_BLOCK_SIZE 1

struct Inode
{
    uint32_t i_no; // inode编号
    uint32_t i_size; // 文件大小
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
    @param p_part : struct Partition * : 扇区信息
    @param inode_no : uint32_t:  inode编号
    @return struct Inode * 打开的inode信息地址
*/
struct Inode* inode_open(struct Partition *p_part, uint32_t inode_no);
#endif