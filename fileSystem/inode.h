#ifndef __FILESYSTEM_INODE_H
#define __FILESYSTEM_INODE_H

#include "stdint.h"
#include "list.h"
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

/*
    @brief 获取inode所在扇区的位置信息
    @param p_part : struct Partition * : 扇区信息
    @param inode_no : uint32_t:  inode编号
    @param p_inode_position: struct Inode_position *: 返回的信息
*/
void inode_locate(struct Partition *p_part, uint32_t inode_no, struct Inode_position *p_inode_pos);
#endif