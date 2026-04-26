#ifndef __FILESYSTEM_FS_H
#define __FILESYSTEM_FS_H
#include "ide.h"
// 分区最大文件数
#define MAX_FILES_PER_PART 4096

// 每扇区的位数
#define BITS_PER_SECTOR (SECTOR_SIZE_BYTE*8)  
#define BLOCK_SIZE SECTOR_SIZE_BYTE

typedef enum File_types {
    FT_UNKNOWN = 0, // 未知
    FT_REGULLAR, // 普通文件
    FT_DIRECTORY, // 目录
} File_types;

/*
@brief 为分区格式化文件系统
@param part: struct Partition * : 分区指针
*/
void partition_format(struct Partition *part);

/*
 @brief 为磁盘上所有未格式化分区创建文件系统，跳过主盘
*/
void fileSystem_init();

/*
@brief 加载指定名字的分区信息到内存里的全局变量，并初始化对应的位图信息等, 旧的分区会卸载
*/
void load_partition(char *part_name);

/*
@brief 加载默认的分区，即第一个
*/
void load_default_partition();
#endif              