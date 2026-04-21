#ifndef __FILESYSTEM_SUPER_BLOCK_H
#define __FILESYSTEM_SUPER_BLOCK_H
#define SUPER_BLOCK_MAGIC_NUMBER 0X19590318

#include "stdint.h"
/*
文件系统扇区安排
低LBA地址  |操作系统引导块|超级块|块位图|inode位图|inode表|根目录|数据块|   高LBA地址
根目录也是数据块，不过要先找到根目录才能找到其他数据，所以根目录是单独列出来的
*/
struct Super_block{
    uint32_t magic; //标识文件系统类型，不过这里就是判断是不是指定数字来判断文件系统在不在
    uint32_t size_sector;//本分区的总扇区数
    uint32_t inode_count;//本分区inode数量
    uint32_t partition_lba_base; //分区的起始LBA地址

    uint32_t block_bitmap_lba;//块位图lba地址
    uint32_t block_bitmap_size_sector; //块位图大小，扇区为单位

    uint32_t inode_bitmap_lba; //inode位图LBA地址
    uint32_t inode_bitmap_size_sector; //inode位图占的扇区大小

    uint32_t inode_table_lba; //inode节点表的LBA地址
    uint32_t inode_table_size_sector; //inode节点表占的扇区大小

    uint32_t data_area_lba_base;//数据区的起始LBA地址
    uint32_t root_inode_no; //根目录的inode号
    uint32_t dir_entry_size; //目录项大小

    uint8_t unused[460];//填充到512, 凑齐一个扇区
} __attribute__ ((packed));

#endif