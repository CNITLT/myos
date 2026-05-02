#ifndef __FILESYSTEM_FS_H
#define __FILESYSTEM_FS_H
#include "ide.h"
// 分区最大文件数
#define MAX_FILES_PER_PART 4096
#define MAX_PATH_LENGTH 512
// 每扇区的位数
#define BITS_PER_SECTOR (SECTOR_SIZE_BYTE*8)  
#define BLOCK_SIZE SECTOR_SIZE_BYTE

typedef enum File_types {
    FT_UNKNOWN = 0, // 未知
    FT_REGULLAR, // 普通文件
    FT_DIRECTORY, // 目录
} File_types;

typedef enum oflags {
    O_RD_ONLY = 0, // 只读
    O_WR_ONLY = 1, // 只写
    O_RDWR = 2, // 读写
    O_CREAT = 4 // 创建
} oflags;

struct Path_search_record {
    char searched_path[MAX_PATH_LENGTH]; // 查找过程中的父路径，主要是在断链没找到的情况下有用
    struct dir* p_parent_dir; //文件或目录所在的直接父目录
    File_types file_type; // 找到的文件类型, 找不到为FT_UNKNOWN
};
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

/*
 * @brief 搜索path对应的文件或目录，找到最终文件，最终目录或者中途断链（/a/b/c b是个文件）则返回最后搜索的inode, 若中间搜索过程存在没找到的情况则返回-1
 * @param path : const char *: 搜索路径
 * @param p_searched_record: struct Path_search_record *: 搜索信息详情
*/
int search_file(const char *path, struct Path_search_record *p_searched_record);
#endif              