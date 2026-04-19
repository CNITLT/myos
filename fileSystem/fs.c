#include "fs.h"
#include "inode.h"
#include "super_block.h"
#include "dir.h"
void partition_format(struct Disk *hd, struct Partition *part) {
    // 目前一个block和sector是等价的
    uint32_t boot_sector_sectors = 1;
    uint32_t super_block_sectors = 1;
    uint32_t inode_bitmap_sectors = DIV_ROUND_UP(MAX_FILES_PER_PART, BITS_PER_SECTOR);
    uint32_t inode_table_sectors = DIV_ROUND_UP((sizeof(struct Inode) * MAX_FILES_PER_PART), SECTOR_SIZE);

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
    struct Super_block super_block;
   
    super_block.magic = SUPER_BLOCK_MAGIC_NUMBER;  // 魔数
    super_block.size_sector = part->size_sector;  // 总扇区数
    super_block.inode_count = MAX_FILES_PER_PART; // inode总数
    super_block.partition_lba_base = part->start_lba; // 起点
    super_block.block_bitmap_lba = super_block.partition_lba_base + 2;     // 块位图起点
    super_block.block_bitmap_size_sector = block_bitmap_sectors; // 块位图大小

    super_block.inode_bitmap_lba = super_block.block_bitmap_lba + super_block.block_bitmap_size_sector; // inode位图起点
    super_block.inode_bitmap_size_sector = inode_bitmap_sectors; // inode位图大小

    super_block.inode_table_lba = super_block.inode_bitmap_lba + super_block.inode_bitmap_size_sector; // inode表起点
    super_block.inode_table_siez_sector = inode_table_sectors; // inode表大小

    super_block.data_area_lba_base = super_block.inode_table_lba + super_block.inode_table_siez_sector; // 数据块起点
    super_block.root_inode_no = 0; 
    super_block.dir_entry_size = sizeof(struct Dir_entry);
}