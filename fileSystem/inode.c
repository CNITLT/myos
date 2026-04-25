#include "inode.h"
#include "ide.h"
#include "stdint.h"
#include "fs.h"
#include "debug.h"
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
void inode_locate(struct Partition *p_part, uint32_t inode_no, struct Inode_position *p_inode_pos) {
    assert(inode_no < MAX_FILES_PER_PART);
    assert(p_inode_pos != NULL);
    // 先计算相对于inodeTable起点的偏移
    const uint32_t inode_offset_in_table = inode_no * sizeof(struct Inode);
    const uint32_t inode_offset_in_sector = inode_offset_in_table % SECTOR_SIZE_BYTE;
    // 剩下的空间不足够容纳一个说明跨页
    p_inode_pos->is_in_two_section = (SECTOR_SIZE_BYTE - inode_offset_in_sector) < sizeof(struct Inode);
    p_inode_pos->offset_in_section = inode_offset_in_sector;
    p_inode_pos->section_lba = p_part->super_block->inode_table_lba + (inode_offset_in_table / sizeof(struct Inode));
}

