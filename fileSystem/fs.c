#include "fs.h"
#include "inode.h"
#include "super_block.h"
#include "dir.h"
#include "print.h"
#include "memory.h"
#include "string.h"
#include "debug.h"

// 默认情况下的操作分区
struct Partition *g_current_part; 

void partition_format(struct Partition *part) {
    // 目前一个block和sector是等价的
    uint32_t boot_sector_sectors = 1;
    uint32_t super_block_sectors = 1;
    uint32_t inode_bitmap_sectors = DIV_ROUND_UP(MAX_FILES_PER_PART, BITS_PER_SECTOR);
    uint32_t inode_table_sectors = DIV_ROUND_UP((sizeof(struct Inode) * MAX_FILES_PER_PART), SECTOR_SIZE_BYTE);

    // 已使用的
    uint32_t used_sectors = boot_sector_sectors + super_block_sectors + inode_bitmap_sectors + inode_table_sectors;
    // 剩余的
    uint32_t free_sectors = part->size_sector - used_sectors;

    // 处理空闲位图，采用比较简单的方式计算
    //  真要按实际公式计算的话，应该是除 BITS_PER_SECTOR + 1，但数量级上差异不大，直接算吧，还简单点
    // 这么算其实有一部分是没有被管理的，不过无所谓了，简单点就简单点把
    uint32_t block_bitmap_sectors = DIV_ROUND_UP(free_sectors, BITS_PER_SECTOR);
    uint32_t block_bitmap_bit_len = free_sectors - block_bitmap_sectors; // 从空闲块减去位图的数量，即长度，有一定的冗余
    // 这里计算的误差，即没有被当成位图，也没有被当成空闲块，是幽灵块了
    block_bitmap_sectors = DIV_ROUND_UP(block_bitmap_bit_len, BITS_PER_SECTOR); 

    // 超级块填写， 按如下结构的视图
/*
    | OBR(1) | superBlock(1) | blockBitMap(?) | inodeBitMap(?)  |  inodeTable(?) | rootDir(视为数据块) | free |
*/
    struct Super_block *p_super_block = sys_malloc(sizeof(struct Super_block));
   
    p_super_block->magic = SUPER_BLOCK_MAGIC_NUMBER;  // 魔数
    p_super_block->size_sector = part->size_sector;  // 总扇区数
    p_super_block->inode_count = MAX_FILES_PER_PART; // inode总数
    p_super_block->partition_lba_base = part->start_lba; // 起点
    p_super_block->block_bitmap_lba = p_super_block->partition_lba_base + 2;     // 块位图起点
    p_super_block->block_bitmap_size_sector = block_bitmap_sectors; // 块位图大小

    p_super_block->inode_bitmap_lba = p_super_block->block_bitmap_lba + p_super_block->block_bitmap_size_sector; // inode位图起点
    p_super_block->inode_bitmap_size_sector = inode_bitmap_sectors; // inode位图大小

    p_super_block->inode_table_lba = p_super_block->inode_bitmap_lba + p_super_block->inode_bitmap_size_sector; // inode表起点
    p_super_block->inode_table_size_sector = inode_table_sectors; // inode表大小

    p_super_block->data_area_lba_base = p_super_block->inode_table_lba + p_super_block->inode_table_size_sector; // 数据块起点
    p_super_block->root_inode_no = 0; 
    p_super_block->dir_entry_size = sizeof(struct Dir_entry);
// --- debug ----
    printf("super_block size_sector:%d\n", p_super_block->size_sector);
    printf("super_block inode_count:%d\n", p_super_block->inode_count);
    printf("super_block partition_lba_base:0x%x\n", p_super_block->partition_lba_base);
    printf("super_block block_bitmap_lba:0x%x\n", p_super_block->block_bitmap_lba);
    printf("super_block block_bitmap_size_sector:0x%x\n", p_super_block->block_bitmap_size_sector);

    printf("super_block inode_bitmap_lba:0x%x\n", p_super_block->inode_bitmap_lba);
    printf("super_block inode_bitmap_size_sector:0x%x\n", p_super_block->inode_bitmap_size_sector);

    printf("super_block inode_table_lba:0x%x\n", p_super_block->inode_table_lba);
    printf("super_block inode_table_size_sector:0x%x\n", p_super_block->inode_table_size_sector);

    printf("super_block data_area_lba_base:0x%x\n", p_super_block->data_area_lba_base);
// --- debug ----

    // 超级块写入
    struct Disk *part_disk = part->p_disk;
    ide_write(part_disk, part->start_lba + 1, p_super_block, 1);
    
    // 申请位图数据库，以最大的为准，之后位图初始化可以重复使用
    uint32_t buf_size = MAX(p_super_block->block_bitmap_size_sector, p_super_block->inode_bitmap_size_sector);
    buf_size = MAX(buf_size, p_super_block->inode_table_size_sector);
    buf_size *= SECTOR_SIZE_BYTE;

    Byte *buff = (Byte *)sys_malloc(buf_size);
    memset(buff, 0, buf_size);

    // blockBitMap写入
    struct bitmap block_bitmap;
    block_bitmap.bits = buff;
    uint32_t real_bit_len = p_super_block->block_bitmap_size_sector;
    block_bitmap.len_bit = DIV_ROUND_UP(real_bit_len, 8) * 8; // 多出一些间距, 免得触发断言
    bitmap_init(&block_bitmap);
    // 第0个块是根目录，先占位
    bitmap_set(&block_bitmap, 0, BIT_STATE_USE);
    // 最后一个字节的处理，bitmap里没有实际对应的块，置1， 笔记最小存储单位是字节，不存在的位置1
    for (uint32_t i = real_bit_len; i < block_bitmap.len_bit; i++) {
       bitmap_set(&block_bitmap, i, BIT_STATE_USE);
    }
    // 写入磁盘
    ide_write(part_disk, p_super_block->block_bitmap_lba, block_bitmap.bits, p_super_block->block_bitmap_size_sector);

    // inodeBitMap 写入
    memset(buff, 0, buf_size);
    struct bitmap inode_bitmap;
    inode_bitmap.bits = buff;
    real_bit_len = p_super_block->inode_bitmap_size_sector;
    inode_bitmap.len_bit = DIV_ROUND_UP(real_bit_len, 8) * 8; // 多出一些间距, 免得触发断言
    bitmap_init(&inode_bitmap);
    // 第0个块是根目录，先占位
    bitmap_set(&inode_bitmap, 0, BIT_STATE_USE);
    // 最后一个字节的处理，bitmap里没有实际对应的块，置1， 笔记最小存储单位是字节，不存在的位置1
    for (uint32_t i = real_bit_len; i < inode_bitmap.len_bit; i++) {
       bitmap_set(&inode_bitmap, i, BIT_STATE_USE);
    }
    // 写入磁盘
    ide_write(part_disk, p_super_block->inode_bitmap_lba, inode_bitmap.bits, p_super_block->inode_bitmap_size_sector);
    //  inodeTable 写入
    memset(buff, 0, buf_size); 
    struct Inode *root_Inode = (struct  Inode *)buff;
    root_Inode->i_no = 0;
    root_Inode->i_size = p_super_block->dir_entry_size * 2;// .和..
    root_Inode->i_sectors[0] = p_super_block->data_area_lba_base;
    ide_write(part_disk, p_super_block->inode_table_lba, buff, p_super_block->inode_table_size_sector); 
    
    // 根目录项目写入 .和..
    memset(buff, 0, buf_size);  
    struct Dir_entry *p_now_entry =  (struct Dir_entry *)buff;
    memcpy(p_now_entry->fileName, ".", 1);
    p_now_entry->i_no = 0;
    p_now_entry->f_type = FT_DIRECTORY;

    struct Dir_entry *p_parent_entry = ++p_now_entry;
    memcpy(p_parent_entry->fileName, "..", 2);
    // 根目录只能指向自己
    p_now_entry->i_no = 0;
    p_now_entry->f_type = FT_DIRECTORY;
    ide_write(part_disk, p_super_block->data_area_lba_base, buff, 1);
    sys_free(buff);
    sys_free(p_super_block);
}


void fileSystem_init() {
    uint32_t channel_no = 0;
    uint32_t dev_no = 0;
    uint32_t part_index = 0;

    struct Super_block *p_super_block = (struct  Super_block *)sys_malloc(SECTOR_SIZE_BYTE);
    assert(p_super_block);
   
    struct list_node* iter = g_partition_list.head.next;
    struct list_node* res = NULL;
    while(iter != &(g_partition_list.tail)){
        // 这里遍历的都是可以写数据的分区，忽略了操作系统代码占的分区
        struct Partition *p_part = elem2entry(struct Partition, part_tag, iter);
        if (p_part->size_sector > 0) {
            ide_read(p_part->p_disk, p_part->start_lba + 1, p_super_block, 1);
            printf("part:%s magic:0x%x size:%d\n", p_part->name, p_super_block->magic, p_part->size_sector);
            if (p_super_block->magic != SUPER_BLOCK_MAGIC_NUMBER) {
                printf("part:%s init fileSystem\n", p_part->name);
                partition_format(p_part);
            } else {
                printf("part:%s has fileSystem skip init\n", p_part->name); 
            }
        }
        iter = iter->next;
    }
    
    sys_free(p_super_block);
}


void load_partition(char *part_name) {
    struct list_node* iter = g_partition_list.head.next;
    struct list_node* res = NULL;
    while(iter != &(g_partition_list.tail)){
        struct Partition *p_part = elem2entry(struct Partition, part_tag, iter);
        if (!strcmp(part_name, p_part->name)) {
            // 相等
            // 没有super_block说明之前没加载过，这里加载一下
            if (!p_part->super_block) {
                // 超级块信息加载
                p_part->super_block = sys_malloc(sizeof(struct Super_block));
                ide_read(p_part->p_disk, p_part->start_lba + 1, p_part->super_block, 1);

                // 位图信息加载
                uint32_t block_bit_map_size = (p_part->super_block->block_bitmap_size_sector) * SECTOR_SIZE_BYTE;
                Byte *p_block_bit_map = sys_malloc(block_bit_map_size);
                ide_read(p_part->p_disk, p_part->super_block->block_bitmap_lba, p_block_bit_map, p_part->super_block->block_bitmap_size_sector);
                p_part->block_bitmap.bits = p_block_bit_map;
                p_part->block_bitmap.len_bit = block_bit_map_size * 8;

                // inode位图信息加载
                uint32_t inode_bit_map_size = (p_part->super_block->inode_bitmap_size_sector) * SECTOR_SIZE_BYTE;
                Byte *p_inode_bit_map = sys_malloc(inode_bit_map_size);
                ide_read(p_part->p_disk, p_part->super_block->inode_bitmap_lba, p_inode_bit_map, p_part->super_block->inode_bitmap_size_sector);
                p_part->inode_bitmap.bits = p_inode_bit_map;
                p_part->inode_bitmap.len_bit = inode_bit_map_size * 8;
                // 链表初始化，其实这里算是重复初始化的，之前有过一次,保险点多一次也无所谓
                list_init(&p_part->opened_inodes);
                printf("load part:%s size sector:%d", p_part->name, p_part->size_sector);
                return;
            } else {
                // 加载过的直接改变指针就行
                g_current_part = p_part;
            }
        }
        iter = iter->next;
    }
}

void load_default_partition() {
    struct list_node* iter = g_partition_list.head.next;
    struct Partition *p_part = elem2entry(struct Partition, part_tag, iter); 
    load_partition(p_part->name);
}