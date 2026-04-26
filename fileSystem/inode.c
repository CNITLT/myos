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

struct Inode* find_opened_inode(struct Partition *p_part, uint32_t inode_no) {
    assert(p_part && inode_no < MAX_FILES_PER_PART);
    struct list_node* iter = p_part->opened_inodes.head.next;
    while(iter != &(p_part->opened_inodes.tail)){
        struct Inode *p_opened_inode = elem2entry(struct Inode, inode_tag, iter);
        if (p_opened_inode->i_no == inode_no) {
            return p_opened_inode;
        }
        iter = iter->next;
    }
    return NULL;
}

struct Inode* inode_open(struct Partition *p_part, uint32_t inode_no) {
    assert(p_part && inode_no < MAX_FILES_PER_PART);
    // 优先从内存里有的找， 找的到就直接返回
    interrupt_state old_intr_state = close_interrupt();
    struct Inode* p_inode = find_opened_inode(p_part, inode_no);
    if (p_inode) {
        // 找到的话打开数+1
        p_inode->i_open_cnts++;
        set_interrupt_state(old_intr_state);
        return p_inode;
    }
    set_interrupt_state(old_intr_state);

    // 找不到就从磁盘打开
    void *io_buff = sys_malloc(2 * SECTOR_SIZE_BYTE);
    // inode给内核关联，分配的空间只能是内存中的
    p_inode = sys_malloc_in_kernel(sizeof(struct Inode));
    struct Inode_position inode_pos;
    inode_locate(p_part, inode_no, &inode_pos);
    ide_read(p_part->p_disk, inode_pos.section_lba, io_buff, inode_pos.is_in_two_section ? 2 : 1);
    struct Inode *p_inode_in_disk = (struct Inode *)io_buff + p_inode->i_no;
    memcpy(p_inode, p_inode_in_disk, sizeof (struct Inode));
    // 部分信息是运行中才有用, 刚读取出来的时候进行一下初始化
    p_inode->i_open_cnts = 1;
    p_inode->write_deny = false;

    // 加入到列表头
    // 先再额外找一次，如果还没有才能加
    old_intr_state = close_interrupt();
    struct Inode* p_ext_find_inode = find_opened_inode(p_part, inode_no);
    if (p_ext_find_inode) {
        // 找到的话打开数+1
        p_ext_find_inode->i_open_cnts++;
        set_interrupt_state(old_intr_state);
        return p_ext_find_inode;
    }
    list_push_front(&p_part->opened_inodes, &p_inode->inode_tag);
    set_interrupt_state(old_intr_state);

    sys_free(io_buff);
    return p_inode;
}

void inode_close(struct Inode* p_inode) {
    interrupt_state old_intr_state = close_interrupt();
    p_inode->i_open_cnts--;
    if (p_inode->i_open_cnts == 0) {
        // 最后一个关闭的从列表中移除
        list_remove(&p_inode->inode_tag);
        sys_free_in_kernel(p_inode);
    }
    set_interrupt_state(old_intr_state);
}

void inode_init(struct Inode* p_inode, uint32_t inode_no) {
    p_inode->i_no = inode_no;
    p_inode->i_size = 0;
    p_inode->i_open_cnts = 0;
    p_inode->write_deny = false;

    for(int i = 0; i < I_NODE_SECTOR_SIZE; i++) {
        p_inode->i_sectors[i] = 0;
    }
}
