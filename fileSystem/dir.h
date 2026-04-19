#ifndef __FILESYSTEM_DIR_H
#define __FILESYSTEM_DIR_H
#include "inode.h"
#include "fs.h"
#define DIR_CACHE_SIZE 512
#define MAX_FILE_NAME_LENGTH 16 


struct Dir
{
    struct  inode* inode;
    uint32_t dir_pos; // 目录内遍历偏移，运行时使用
    Byte dir_buff[DIR_CACHE_SIZE]; // 目录缓存
};

struct Dir_entry {
    char fileName[MAX_FILE_NAME_LENGTH]; // 文件名
    uint32_t i_no; // inode节点编号
    enum file_type f_type;
};

#endif