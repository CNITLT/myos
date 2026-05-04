#ifndef __FILESYSTEM_FS_H
#define __FILESYSTEM_FS_H
#include "ide.h"
// 分区最大文件数
#define MAX_FILES_PER_PART 4096
#define MAX_PATH_LENGTH 512
// 每扇区的位数
#define BITS_PER_SECTOR (SECTOR_SIZE_BYTE*8)  
#define BLOCK_SIZE SECTOR_SIZE_BYTE


extern struct Partition *g_current_part; 

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

typedef enum Whence {
    SEEK_SET = 1, // 开头
    SEEK_CUR, // 当前位置
    SEEK_END //结尾
} Whence;

struct Path_search_record {
    char searched_path[MAX_PATH_LENGTH]; // 查找过程中的父路径，主要是在断链没找到的情况下有用
    struct Dir* p_parent_dir; //文件或目录所在的直接父目录, 由调用函数记得释放, 被掉函数打开
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
 * @param path:const char *: 路径 /a/b/c这类
 * @return int32_t 路径深度
*/
int32_t path_depth(const char *path);

/*
 * @brief 搜索path对应的文件或目录，找到最终文件，最终目录或者中途断链（/a/b/c b是个文件）则返回最后搜索的inode, 若中间搜索过程存在没找到的情况则返回-1
 * @param path : const char *: 搜索路径
 * @param p_searched_record: struct Path_search_record *: 搜索信息详情
*/
int search_file(const char *path, struct Path_search_record *p_searched_record);


/*
 * @brief 创建文件，成功返回文件描述符(进程级), 否则-1, 
 * @param p_dir : struct Dir *: 待创建文件所在的目录
 * @param filename: char *: 文件名，有长度限制
 * @param flag : uint8_t : 文件权限 
 * @return 成功则返回进程内的文件描述符，否则-1
 * @note 内部没有先搜索是否同名文件已经存在, 需要外部判断
*/
int32_t file_create(struct Dir *p_dir, char *filename, uint8_t flag);

/*
 * @brief 打开或创建文件，成功返回文件描述符，否则-1
 * @param path: const char *: 文件绝对路径
 * @param flags: uint8_t: 对应的操作权限
 * @return int32_t 进程级文件描述符
 * @note 只能打开文件
*/
int32_t sys_open(const char *path, uint8_t flags);

/*
 * @brief 写入数据到特定的文件描述符
 * @param fd: int32_t :待写入的文件描述符索引
 * @param data: void *: 待写入的数据 
 * @param count: size_t :写入的数据量
 * @return 成功返回写入的数据量，失败返回-1
*/
int32_t sys_write(int32_t fd, const void *data, size_t count);


/*
 * @brief 读取文件描述符的数据
 * @param fd: int32_t :待读取的文件描述符索引
 * @param data: void *: 待读取的数据的缓冲区
 * @param count: size_t :读取的数据量
 * @return 成功返回读取的数据量，失败返回-1
*/
int32_t sys_read(int32_t fd, void *data, size_t count);

/*
 * @brief 调整文件描述符里的游标
 * @param fd: int32_t :待读取的文件描述符索引
 * @param offset: int32_t : 偏移量
 * @param whence: uint8_t : 枚举值，偏移的起点
 * @return int32_t 新的游标位置, 失败返回-1
*/
int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence);

/*
 * @brief 删除非目录类型的文件
 * @param path: const char *: 文件路径
 * @return 成功返回0， 否则-1
*/
int32_t sys_unlink(const char *path);

/*
 * @brief 当前进程文件描述符转化为全局文件描述符
 * @param local_fd_index: uin32_t : 当前进程的一个文件描述符
 * @return 成功返回全局的文件描述符
*/
int32_t fd_local2global(uint32_t local_fd_index);

#endif              