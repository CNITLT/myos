#include "inode.h"
#include "ide.h"
#include "stdint.h"
#include "fs.h"
#include "debug.h"
#include "memory.h"
#include "string.h"
#include "file.h"

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
    struct Inode *p_inode_in_disk = (struct Inode *)((Byte *)io_buff + inode_pos.offset_in_section);
    memcpy(p_inode_in_disk, p_inode, sizeof(struct Inode));
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
    // printf("debug find_opened_inode start\n");
    // while(1){};
    struct list_node* iter = p_part->opened_inodes.head.next;
    while(iter != &(p_part->opened_inodes.tail)){
        struct Inode *p_opened_inode = elem2entry(struct Inode, inode_tag, iter);
        if (p_opened_inode->i_no == inode_no) {
            return p_opened_inode;
        }
        iter = iter->next;
    }
    // printf("debug find_opened_inode end\n");
    // while(1){};
    return NULL;
}

struct Inode* inode_open(struct Partition *p_part, uint32_t inode_no) {
    assert(p_part && inode_no < MAX_FILES_PER_PART);
    // 优先从内存里有的找， 找的到就直接返回
    // printf("debug inode_open start\n");
    interrupt_state old_intr_state = close_interrupt();
    struct Inode* p_inode = find_opened_inode(p_part, inode_no);
    if (p_inode) {
        // 找到的话打开数+1
        p_inode->i_open_cnts++;
        set_interrupt_state(old_intr_state);
        return p_inode;
    }
    set_interrupt_state(old_intr_state);
    // printf("debug inode_open opened not exit will find in disk\n");
    // while (1);
    
    // 找不到就从磁盘打开
    void *io_buff = sys_malloc(2 * SECTOR_SIZE_BYTE);
    // inode给内核关联，分配的空间只能是内存中的
    p_inode = sys_malloc_in_kernel(sizeof(struct Inode));

    struct Inode_position inode_pos;
    inode_locate(p_part, inode_no, &inode_pos);
    ide_read(p_part->p_disk, inode_pos.section_lba, io_buff, inode_pos.is_in_two_section ? 2 : 1);
    struct Inode *p_inode_in_disk = (struct Inode *)((Byte *)io_buff + inode_pos.offset_in_section);
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


void get_inode_all_block_lba(struct Partition* p_part, struct Inode *p_inode, uint32_t **p_all_block_lba_ret, uint32_t *p_all_block_lba_count_ret) {
    assert(BLOCK_SIZE % sizeof(uint32_t) == 0);
    // 对p_all_block_lba 的初始化感觉效率有点低，但无所谓了
    uint32_t all_block_count = I_NODE_LAYER0_BLCOK_SIZE + I_NODE_LAYER1_BLOCK_SIZE * BLOCK_SIZE / sizeof(uint32_t);
    uint32_t *p_all_block_lba = (uint32_t  *)sys_malloc(all_block_count * sizeof(uint32_t));
    // 这里直接假定一定成功，不想考虑太多了
    assert(p_all_block_lba);
    uint32_t *p_block_iter = p_all_block_lba;
    for (int i = 0; i < I_NODE_LAYER0_BLCOK_SIZE; i++) {
        *p_block_iter = p_inode->i_sectors[i];
        p_block_iter++;
    }
    Byte *buff = (Byte *)sys_malloc(BLOCK_SIZE);
    for (int i = I_NODE_LAYER0_BLCOK_SIZE; i < I_NODE_LAYER0_BLCOK_SIZE + I_NODE_LAYER1_BLOCK_SIZE; i++) {
        int j = I_NODE_LAYER0_SIZE_PER_LAYER1;
        if (p_inode->i_sectors[i] == 0)  {
            while(j--) {
                *p_block_iter = 0;
                p_block_iter++;
            }
        } else {
            ide_read(p_part->p_disk, p_inode->i_sectors[i], buff, 1);
            uint32_t *p_block_iter_src = buff;
            while(j--) {
                *p_block_iter = *p_block_iter_src;
                p_block_iter++;
                p_block_iter_src++;
            }
        }
    }
    *p_all_block_lba_ret = p_all_block_lba;
    *p_all_block_lba_count_ret = all_block_count;
    sys_free(buff);
}

// 在所有直接块索引转化到sector_index索引，主要是用于一级块索引定位
static int32_t all_block_index2_i_sector_index(int32_t all_block_index) {
    // 直接块不需要算，直接返回原值
    if (all_block_index < I_NODE_LAYER0_BLCOK_SIZE) {
        return all_block_index;
    } else {
        return I_NODE_LAYER0_BLCOK_SIZE + (all_block_index - I_NODE_LAYER0_BLCOK_SIZE) / I_NODE_LAYER0_SIZE_PER_LAYER1;
    }
}

/*
  @brief 分配直接块或者1级块，即对i_sector分配块
  @param p_part: struct Partition* :操作扇区
  @param p_inode: struct Inode * :inode节点
  @param i_sector_index: int32_t :待分配的i_sector索引
  @return 已分配的block_lba地址, 若已经存在则返回已经存在的值
 */
static int32_t alloc_inode_sector_block(struct Partition* p_part, struct Inode *p_inode,  int32_t i_sector_index) {
    assert(i_sector_index < I_NODE_SECTOR_SIZE);
    // printf("debug alloc_inode_sector_block p_inode_no:%d i_sectors[0]:0x%x\n", p_inode->i_no, p_inode->i_sectors[0]);
    if (p_inode->i_sectors[i_sector_index]) {
        return p_inode->i_sectors[i_sector_index];
    }
    int32_t block_lba = block_bitmap_alloc(p_part);
    if (block_lba == -1) {
        printf("alloc_inode_layer0_block block_bitmap_alloc false");
        return -1;
    }
    // 然后同步磁盘
    int32_t block_bitmap_index = block_lba - p_part->super_block->data_area_lba_base;
    assert(block_bitmap_index != -1);
    bitmap_sync(p_part, block_bitmap_index, Bitmap_type_block);
    p_inode->i_sectors[i_sector_index] = block_lba;
    // 同步inode
    inode_sync(p_part, p_inode, NULL);
    // 重置下磁盘的内容
    Byte *buff = sys_malloc(BLOCK_SIZE);
    if (buff) {
        memset(buff, 0, BLOCK_SIZE);
        ide_write(p_part->p_disk, block_lba, buff, 1);
        sys_free(buff);
    }
    
    return block_lba;
}

// 分配直接块
static int32_t alloc_inode_layer0_block(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t p_all_block_lba_count, int32_t all_block_index) {
    int32_t i_sector_index = all_block_index2_i_sector_index(all_block_index);
    int32_t block_lba = alloc_inode_sector_block(p_part, p_inode,i_sector_index);
    if (block_lba == -1) {
        printf("alloc_inode_layer0_block faild\n");
        return -1;
    }
    p_all_block_lba[all_block_index] = block_lba;
    return block_lba;
}


// 分配1级块, 若i_sector内的一级块没有分配，也会同时分配
static int32_t alloc_inode_layer1_block(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t p_all_block_lba_count, int32_t all_block_index) {
    assert(all_block_index >= I_NODE_LAYER0_BLCOK_SIZE);
    int32_t i_sector_index = all_block_index2_i_sector_index(all_block_index);
    int32_t layer1_block_lba = p_inode->i_sectors[i_sector_index];
    if (layer1_block_lba == 0) {
        // 次数先分配i_sector 一级块
        layer1_block_lba = alloc_inode_sector_block(p_part, p_inode, i_sector_index);
        if (layer1_block_lba == -1) {
            printf("alloc_inode_layer1_block  alloc layer1 block faild\n");
            return -1;
        }
    }

    Byte *buff = sys_malloc(BLOCK_SIZE);
    if (!buff) {
        printf("alloc_inode_layer1_block  sys_malloc buff faild\n");
        return -1; 
    }
    ide_read(p_part->p_disk, layer1_block_lba, buff, 1);
    int32_t *p_block = ((int32_t *)buff + (all_block_index - I_NODE_LAYER0_BLCOK_SIZE) % I_NODE_LAYER0_SIZE_PER_LAYER1);
    int32_t block_lba = *p_block;
    if (block_lba == 0) {
        // 不存在就分配一个
        block_lba = block_bitmap_alloc(p_part);
        if (block_lba == -1) {
            printf("alloc_inode_layer1_block block_bitmap_alloc block_lba faild\n");
            sys_free(buff);
            return false;
        }
        // 分配成功，先把bitmap写入
        int32_t block_bitmap_index = block_lba - p_part->super_block->data_area_lba_base;
        assert(block_bitmap_index != -1);
        bitmap_sync(p_part, block_bitmap_index, Bitmap_type_block);

        // 写入一级块
        *p_block = block_lba;
        ide_write(p_part->p_disk, layer1_block_lba, buff, 1);

        // 写入inode
        inode_sync(p_part, p_inode, NULL);
        
        // 刚分配的直接块内容清0
        memset(buff, 0, BLOCK_SIZE);
        ide_write(p_part->p_disk, block_lba, buff, 1);
    }

    sys_free(buff);
    p_all_block_lba[all_block_index] = block_lba;
    return block_lba;        
}


int32_t alloc_inode_all_block(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t p_all_block_lba_count, int32_t all_block_index) {
     int32_t i_sector_index = all_block_index2_i_sector_index(all_block_index);
     if (i_sector_index < I_NODE_LAYER0_BLCOK_SIZE) {
        return alloc_inode_layer0_block(p_part, p_inode, p_all_block_lba, p_all_block_lba_count, all_block_index);
     } else {
        return alloc_inode_layer1_block(p_part, p_inode, p_all_block_lba, p_all_block_lba_count, all_block_index);
     }
}


/*
  @brief 释放直接块或者1级块，即对i_sector分配块
  @param p_part: struct Partition* :操作扇区
  @param p_inode: struct Inode * :inode节点
  @param i_sector_index: int32_t :待释放的i_sector索引
  @return 只会返回成功0
 */
static int32_t free_inode_sector_block(struct Partition* p_part, struct Inode *p_inode,  int32_t i_sector_index) {
    assert(i_sector_index < I_NODE_SECTOR_SIZE);
    // 块不存在直接当释放了
    if (p_inode->i_sectors[i_sector_index] == 0) {
        return 0;
    }
    int32_t block_lba = p_inode->i_sectors[i_sector_index];
    block_bitmap_free(p_part, block_lba);
    // 然后同步磁盘
    int32_t block_bitmap_index = block_lba - p_part->super_block->data_area_lba_base;
    assert(block_bitmap_index != -1);
    bitmap_sync(p_part, block_bitmap_index, Bitmap_type_block);
    p_inode->i_sectors[i_sector_index] = 0;
    // 同步inode
    inode_sync(p_part, p_inode, NULL);
    return 0;
}


// 释放直接块
static int32_t free_inode_layer0_block(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t p_all_block_lba_count, int32_t all_block_index) {
    assert(all_block_index < I_NODE_LAYER0_BLCOK_SIZE);
    int32_t i_sector_index = all_block_index2_i_sector_index(all_block_index);
    int res = free_inode_sector_block(p_part, p_inode, i_sector_index);
    p_all_block_lba[all_block_index] = 0;
    return res;
}

// 释放1级块, 一级块内没有直接块指向，则释放一级块, 若块本身没有被分配，则认为释放成功
static int32_t free_inode_layer1_block(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t p_all_block_lba_count, int32_t all_block_index) {
    assert(all_block_index >= I_NODE_LAYER0_BLCOK_SIZE);
    int32_t block_lba = p_all_block_lba[all_block_index];
    if (block_lba == 0) {
        return 0;
    }
    // 先释放直接块
    block_bitmap_free(p_part, block_lba);
    // 然后同步磁盘
    int32_t block_bitmap_index = block_lba - p_part->super_block->data_area_lba_base;
    assert(block_bitmap_index != -1);
    bitmap_sync(p_part, block_bitmap_index, Bitmap_type_block);
    p_all_block_lba[all_block_index] = 0;
    // 再检查下一级块内有无其他直接块
    bool has_other_block = false;
    int32_t start_block_index = I_NODE_LAYER0_BLCOK_SIZE + (all_block_index2_i_sector_index(all_block_index) - I_NODE_LAYER0_BLCOK_SIZE) * I_NODE_LAYER0_SIZE_PER_LAYER1;
    for (int i = start_block_index; i < start_block_index + I_NODE_LAYER0_SIZE_PER_LAYER1; i++) {
        if (p_all_block_lba[i] != 0) {
            has_other_block = true;
            break;
        }
    }

    if (!has_other_block) {
        // 如果没有其他直接块的话，同时把i_sector里的也释放了
        int32_t i_sector_index = all_block_index2_i_sector_index(all_block_index);
        free_inode_sector_block(p_part, p_inode, i_sector_index);
    }
    return 0;
}


int32_t free_inode_all_block(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t p_all_block_lba_count, int32_t all_block_index) {
     int32_t i_sector_index = all_block_index2_i_sector_index(all_block_index);
     if (i_sector_index < I_NODE_LAYER0_BLCOK_SIZE) {
        return free_inode_layer0_block(p_part, p_inode, p_all_block_lba, p_all_block_lba_count, all_block_index);
     } else {
        return free_inode_layer1_block(p_part, p_inode, p_all_block_lba, p_all_block_lba_count, all_block_index);
     }
}