#include "inode.h"
#include "ide.h"
#include "stdint.h"
#include "fs.h"
#include "debug.h"
#include "memory.h"
#include "string.h"

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

void inode_sync(struct Partition *p_part, struct Inode *p_inode, void *io_buff) {
    assert(p_part && p_inode);
    bool need_free = false;
    if (!io_buff) {
        io_buff = sys_malloc(2 * SECTOR_SIZE_BYTE);
        need_free = true;
    }
    struct Inode_position inode_pos;
    inode_locate(p_part, p_inode->i_no, &inode_pos);
    ide_read(p_part->p_disk, inode_pos.section_lba, io_buff, inode_pos.is_in_two_section ? 2 : 1);
    struct Inode *p_inode_in_disk = (struct Inode *)io_buff + p_inode->i_no;
    memcpy(p_inode_in_disk, p_inode, sizeof (struct Inode));
    // 部分信息是运行中才有用的，直接清空
    p_inode_in_disk->inode_tag.prev = NULL;
    p_inode_in_disk->inode_tag.next = NULL;
    p_inode_in_disk->i_open_cnts = 0;
    p_inode_in_disk->write_deny = false;
    ide_write(p_part->p_disk, inode_pos.section_lba, io_buff, inode_pos.is_in_two_section ? 2 : 1);
    if (need_free) {
        sys_free(io_buff);
    }
}