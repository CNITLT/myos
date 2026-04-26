#ifndef __FILESYSTEM_FILE_H
#define __FILESYSTEM_FILE_H
#include "stdint.h"
#include "inode.h"
#include "ide.h"
// 整个系统所有进程同时打开的文件描述符数最大值
// 同文件多次打开，算不同文件描述符
#define MAX_FD_SIZE 256
// 可用的文件描述符开头, 标准输入 输出 错误输出占了前3个，其他的只能从3开始了
#define USED_FD_START_INDEX 3
struct File
{
    uint32_t fd_pos; // 当前文件操作偏移，从0开始
    uint32_t fd_flag; // 文件操作权限 读写
    struct Inode *p_fd_inode; // 文件对应的inode节点
};

// 标准输入输出描述符
enum Std_fd {
    stdin_no = 0,
    stdout_no = 1,
    stderr_no = 2
};

enum Bitmap_type {
    Bitmap_type_inode, // inode位图
    Bitmap_type_block // 块位图
};

/*
    @brief 从全局的文件描述符数组里获取一个空闲位, 失败返回-1
    @return uint32_t 可用的数组下标，识别则为-1
*/
int32_t get_free_file_slot_in_g_table();

/*
    @brief 安装分配的全局文件描述符索引到当前线程的PCB的文件描述符数组内
    @return int32_t 失败返回-1， 否则返回安装位置的索引
*/
int32_t pcb_fd_install(int32_t globa_fd_index);

/*
    @brief 从位图内分配inode节点，返回节点编号
    @param p_part: struct Partition *: 分区信息
    @return uint32_t 节点编号，失败返回-1
*/
int32_t inode_bitmap_alloc(struct Partition *p_part);


/*
    @brief 从位图内分配block节点，返回节点对应的扇区地址
    @param p_part: struct Partition *: 分区信息
    @return uint32_t 扇区地址，失败返回-1
*/
int32_t block_bitmap_alloc(struct Partition *p_part)
#endif