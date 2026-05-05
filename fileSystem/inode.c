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
    // 先从bitmap看看这个inode有无被分配
    bit_state inode_state = bitmap_get(&p_part->inode_bitmap, inode_no);
    if (inode_state == BIT_STATE_UNUSE) {
        return NULL;
    }


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
    if (!p_inode) {
        return;
    }
    interrupt_state old_intr_state = close_interrupt();
    p_inode->i_open_cnts--;
    if (p_inode->i_open_cnts == 0) {
        // 最后一个关闭的从列表中移除
        list_remove(&p_inode->inode_tag);
        sys_free_in_kernel(p_inode);
    }
    set_interrupt_state(old_intr_state);
}


bool inode_release(struct Partition *p_part, uint32_t inode_no) {
    assert(inode_no < MAX_FILES_PER_PART);
    // 先打开inode
    struct Inode *p_inode = inode_open(p_part, inode_no);
    // 打开失败，说明是不存在的inode
    if (!p_inode) {
        printf("inode_release release a not exist inode, failed\n");
        return false;
    }
    // 其他进程也在打开就不能删
    if (p_inode->i_open_cnts > 1) {
        printf("inode_release release a opened inode, failed\n");
        return false;
    }
    p_inode->write_deny = true;
    uint32_t *p_all_block_lba = NULL;
    uint32_t all_block_lba_count = 0;
    get_inode_all_block_lba(p_part, p_inode, &p_all_block_lba, &all_block_lba_count);
    
    for(int i = all_block_lba_count - 1; i >= 0; i--) {
        free_inode_all_block(p_part, p_inode, p_all_block_lba, all_block_lba_count, i);
    }
    sys_free(p_all_block_lba);
    // 块释放完成开始对inode进行释放
    inode_bitmap_free(p_part, inode_no);
    bitmap_sync(p_part, inode_no, Bitmap_type_inode);
    inode_close(p_inode);
    return true;
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
static int32_t alloc_inode_layer0_block(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t all_block_lba_count, int32_t all_block_index) {
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
static int32_t alloc_inode_layer1_block(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t all_block_lba_count, int32_t all_block_index) {
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


int32_t alloc_inode_all_block(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t all_block_lba_count, int32_t all_block_index) {
     int32_t i_sector_index = all_block_index2_i_sector_index(all_block_index);
     if (i_sector_index < I_NODE_LAYER0_BLCOK_SIZE) {
        return alloc_inode_layer0_block(p_part, p_inode, p_all_block_lba, all_block_lba_count, all_block_index);
     } else {
        return alloc_inode_layer1_block(p_part, p_inode, p_all_block_lba, all_block_lba_count, all_block_index);
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
static int32_t free_inode_layer0_block(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t all_block_lba_count, int32_t all_block_index) {
    assert(all_block_index < I_NODE_LAYER0_BLCOK_SIZE);
    int32_t i_sector_index = all_block_index2_i_sector_index(all_block_index);
    int res = free_inode_sector_block(p_part, p_inode, i_sector_index);
    p_all_block_lba[all_block_index] = 0;
    return res;
}

// 释放1级块, 一级块内没有直接块指向，则释放一级块, 若块本身没有被分配，则认为释放成功
static int32_t free_inode_layer1_block(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t all_block_lba_count, int32_t all_block_index) {
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

int32_t free_inode_all_block(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t all_block_lba_count, int32_t all_block_index) {
     int32_t i_sector_index = all_block_index2_i_sector_index(all_block_index);
     if (i_sector_index < I_NODE_LAYER0_BLCOK_SIZE) {
        return free_inode_layer0_block(p_part, p_inode, p_all_block_lba, all_block_lba_count, all_block_index);
     } else {
        return free_inode_layer1_block(p_part, p_inode, p_all_block_lba, all_block_lba_count, all_block_index);
     }
}


int32_t read_data_from_inode(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t all_block_lba_count, int32_t pos, void *data, size_t count) {
    // printf("debug read_data_from_inode p_part:0x%x p_inode:0x%x data:0x%x\n", p_part, p_inode, data);
    assert(p_part && p_inode && data);
    if (pos < 0 || pos > p_inode->i_size) {
        printf("read_data_from_inode pos is invaild\n");
        return -1;
    }

    // 修正下，如果剩下的可读取的数据量少，以少的为准
    count = MIN(count, (p_inode->i_size - pos));
    
    if (count == 0) {
        // 没有东西可以读了，返回-1
        printf("read_data_from_inode no data could read \n");
        return -1;
    }

    Byte *buff = (Byte *)sys_malloc(BLOCK_SIZE);
    if (!buff) {
        printf("read_data_from_inode malloc buff faild\n");
        return -1;
    }
    int32_t read_count = 0;
    
    // 未读取数据的指针
    Byte *next = (Byte *)data;
    // 还剩下多少未读取
    size_t next_count = count;
    while(next_count) {
         // 操作的相关偏移
        int32_t all_block_write_index = pos / BLOCK_SIZE;
        int32_t index_in_sector = pos % BLOCK_SIZE;
        // 当前可读的剩余容量
        int32_t free_size_in_sector = BLOCK_SIZE - index_in_sector;
        // 本次实际读取的大小
        int32_t read_size_in_once = MIN(free_size_in_sector, next_count);
       
        // 是读取，基本上是一定会有的，而不是走内部的分配
        int32_t block_lba = alloc_inode_all_block(p_part, p_inode, p_all_block_lba, all_block_lba_count, all_block_write_index);
        // printf("debug read_data_from_inode all_block_write_index:%d index_in_sector:%d free_size_in_sector:%d read_size_in_once:%d block_lba:0x%x\n", all_block_write_index,index_in_sector, free_size_in_sector, read_size_in_once,block_lba);
        if (block_lba == -1) {
            printf("file_read alloc_inode_all_block faild\n");
            return read_count;
        }

        ide_read(p_part->p_disk, block_lba, buff, 1);
        // printf("debug read_data_from_inode buff:%s",buff);
        memcpy(next, buff + index_in_sector, read_size_in_once);
     
        // 更新下数据
        pos += read_size_in_once;
        next += read_size_in_once;
        next_count -= read_size_in_once;
        read_count += read_size_in_once;
    }

    sys_free(buff);
    return read_count;
}


int32_t write_data_to_inode(struct Partition* p_part, struct Inode *p_inode, uint32_t *p_all_block_lba, uint32_t all_block_lba_count, int32_t pos, void *data, size_t count) {
    // printf("debug write_data_to_inode p_part:0x%x p_inode:0x%x data:0x%x\n", p_part, p_inode, data);
    assert(p_part && p_inode && data);

    if (pos < 0 || pos > p_inode->i_size) {
        printf("write_data_to_inode pos:%d i_size:%d is invaild\n", pos, p_inode->i_size);
        return -1;
    }

    if (p_inode->i_size + count > MAX_FILE_CONTENT_SIZE) {
        // 超出写入范围了
        printf("write_data_to_inode exceed max content size\n");
        return -1;
    }
    Byte *buff = sys_malloc(2 * BLOCK_SIZE);
    if (!buff) {
        printf("write_data_to_inode malloc buff faild\n");
        return -1;
    }

    int32_t write_count = 0; // 写入量

    // 未写入数据的指针
    Byte *next = (Byte *)data;
    // 还剩下多少未写入
    size_t next_count = count;
    
    while(write_count < count) {
         // 操作的相关偏移
        int32_t all_block_write_index = pos / BLOCK_SIZE;
        int32_t index_in_sector = pos % BLOCK_SIZE;
        // 当前可写的剩余容量
        int32_t free_size_in_sector = BLOCK_SIZE - index_in_sector;
        // 本次实际写入的大小
        int32_t write_size_in_once = MIN(free_size_in_sector, next_count);
        // printf("debug write_data_to_inode i_size:%d \n", p_inode->i_size);
        // 存在的会就是读取，不存在的会先分配然后返回
        int32_t block_lba = alloc_inode_all_block(p_part, p_inode, p_all_block_lba, all_block_lba_count, all_block_write_index);
        if (block_lba == -1) {
            printf("write_data_to_inode alloc_inode_all_block faild\n");
            return write_count;
        }
        if (index_in_sector != 0 || write_size_in_once < BLOCK_SIZE) {
            ide_read(p_part->p_disk, block_lba, buff, 1);
        }
        memcpy(((Byte *)buff + index_in_sector), next, write_size_in_once);
        ide_write(p_part->p_disk, block_lba, buff, 1);

        // 更新下数据
        pos += write_size_in_once;
        next += write_size_in_once;
        next_count -= write_size_in_once;
        write_count += write_size_in_once;
        // 同步下inode
        memset(buff, 0, 2 * BLOCK_SIZE);
        if (pos > p_inode->i_size) {
            p_inode->i_size = pos;
            inode_sync(p_part, p_inode, buff);
        }
    }

    sys_free(buff);
    return write_count;
}
