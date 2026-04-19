#ifndef __FILESYSTEM_FS_H
#define __FILESYSTEM_FS_H
#include "ide.h"
// 分区最大文件数
#define MAX_FILES_PER_PART 4096

// 扇区字节数
#define SECTOR_SIZE 512 
// 每扇区的位数
#define BITS_PER_SECTOR (SECTOR_SIZE*8)  
#define BLOCK_SIZE SECTOR_SIZE

enum File_types {
    FT_UNKNOWN = 0, // 未知
    FT_REGULLAR, // 普通文件
    FT_DIRECTORY, // 目录
};

/*
@brief 为分区格式化文件系统
@param hd: struct Disk  * :磁盘指针
@param part: struct Partition * : 分区指针
*/
void partition_format(struct Disk *hd, struct Partition *part);

#endif              