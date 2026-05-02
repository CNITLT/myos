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

/*
 * @brief 解析路径，例给出/a/b/c 这top_name_buff存的是a, 返回的是/b/c
 * @param path: char *: 路径 /a/b/c这类
 * @param top_name: char *: 给定一个存储空间，返回顶层目录名
 * @return char *: 后序子路径, 如果没有，则为NULL
*/
char *path_parse(char *path, char *top_name_buff);

/*
 * @brief 计算路径深度，例给出/a/b/c 则返回3
 * @param path: char *: 路径 /a/b/c这类
 * @return int32_t 路径深度
*/
int32_t path_depth(char *path);
#endif              