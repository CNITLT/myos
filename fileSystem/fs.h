#ifndef __FILESYSTEM_FS_H
#define __FILESYSTEM_FS_H
// 分区最大文件数
#define MAX_FILES_PER_PART 4096

// 扇区字节数
#define SECTOR_SIZE 512 
// 每扇区的位数
#define SECTOR_BITS_SIZE (SECTOR_SIZE*8)  
#define BLOCK_SIZE SECTOR_SIZE

enum file_types {
    FT_UNKNOWN = 0, // 未知
    FT_REGULLAR, // 普通文件
    FT_DIRECTORY, // 目录
}

#endif