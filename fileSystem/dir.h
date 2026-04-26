#ifndef __FILESYSTEM_DIR_H
#define __FILESYSTEM_DIR_H
#include "inode.h"
#include "fs.h"
#include "ide.h"
#define DIR_CACHE_SIZE 512
#define MAX_FILE_NAME_LENGTH 16 


struct Dir {
    struct Inode* inode;
    uint32_t dir_pos; // 目录内遍历偏移，运行时使用
    Byte dir_buff[DIR_CACHE_SIZE]; // 目录缓存
};

// 单个扇区最后一些位置存不下一个项目的话就新开一个扇区存, 操作起来方便一些
struct Dir_entry {
    char fileName[MAX_FILE_NAME_LENGTH]; // 文件名
    uint32_t i_no; // inode节点编号
    enum File_types f_type;
};

/*
    @brief 打开根目录
    @param p_part: struct Partition *: 分区信息
*/
void open_root_dir(struct Partition* p_part);

/*
    @brief 打开分区上节点为inode_no的目录并返回目录指针，需要释放
    @param p_part: struct Partition *: 分区信息
    @param inode_no : uint32_t: inode编号
    @return struct Dir *: 目录信息，动态分配的内存空间，需要释放
*/
struct Dir *dir_open(struct Partition* p_part, uint32_t inode_no);


/*
    @brief 获取目录下在磁盘所有block的lba地址汇总
    @param p_part: struct Partition *: 分区信息
    @param p_dir:  struct Dir * :搜寻的目录
    @param p_all_block_lba_ret: uint32_t **: 返回值，存储函数分配的所有block lba地址的数组地址的指针地址, 使用完后由外部释放
    @param p_all_block_lba_count_ret: uint32_t *: 返回值，all_block_lba数组元素个数
*/
void get_dir_all_block_lba(struct Partition* p_part, struct Dir *p_dir, uint32_t **p_all_block_lba_ret, uint32_t *p_all_block_lba_count_ret);

/*
    @brief 查找目录下的是否存在名为${entry_name}的目录项，如有则存入p_dir_entry指向的地址内
    @param p_part: struct Partition *: 分区信息
    @param p_dir:  struct Dir * :搜寻的目录
    @param entry_name: char *: 目标名字
    @param p_dir_entry: struct Dir_entry* : 结果存储的地址
    @return bool 成功则返回true 否则false
*/
bool search_dir_entry(struct Partition* p_part, struct Dir *p_dir, char *entry_name, struct Dir_entry* p_dir_entry);

#endif