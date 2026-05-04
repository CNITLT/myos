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
struct File {
    uint32_t fd_pos; // 当前文件操作偏移，从0开始
    uint32_t fd_flag; // 文件操作权限 读写
    struct Inode *p_fd_inode; // 文件对应的inode节点
};

extern struct File g_file_table[MAX_FD_SIZE];

// 标准输入输出描述符
enum Std_fd {
    stdin_no = 0,
    stdout_no = 1,
    stderr_no = 2
};

typedef enum Bitmap_type {
    Bitmap_type_inode, // inode位图
    Bitmap_type_block // 块位图
} Bitmap_type;

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
    @brief 从位图内释放inode节点
    @param p_part: struct Partition *: 分区信息
    @param inode_no : int32_t : inode编号
*/
void inode_bitmap_free(struct Partition *p_part, int32_t inode_no);

/*
    @brief 从位图内分配block节点，返回节点对应的扇区地址
    @param p_part: struct Partition *: 分区信息
    @return int32_t 扇区地址，失败返回-1
*/
int32_t block_bitmap_alloc(struct Partition *p_part);

/*
    @brief 从位图内释放block节点
    @param p_part: struct Partition *: 分区信息
   	@param block_lba : int32_t : 扇区地址 
*/
void block_bitmap_free(struct Partition *p_part, int32_t block_lba);

/*
    @brief 将bit_index对应的那部分bitmap信息同步到磁盘
    @param p_part: struct Partition * : 分区信息
    @param bit_index: int32_t : 改动的数据位置索引
    @param bitmap_type : Bitmap_type : 位图类型
*/
void bitmap_sync(struct Partition *p_part, int32_t bit_index, Bitmap_type bitmap_type);

/*
 * @brief 打开文件，成功返回文件描述符，否则-1
 * @param inode_no : uint32_t : inode编号
 * @param flags: uint8_t: 对应的操作权限
 * @return int32_t 进程级文件描述符
 * @note 只能打开文件
*/
int32_t file_open(uint32_t inode_no, uint8_t flag);

/*
 * @brief 关闭文件
 * @param p_file: struct File * :待关闭的文件描述符
 * @return int32_t 成功返回0，失败-1，基本上只能是0
*/
int32_t file_close(struct File *p_file);

/*
 * @brief 追加写入数据到文件
 * @param p_file: struct File * :待写入的文件描述符
 * @param data: void *: 待写入的数据 
 * @param count: size_t :写入的数据量
 * @return 成功返回写入的数据量，失败返回-1
*/
int32_t file_write(struct File *p_file, void *data, size_t count);

/*
 * @brief 从当前文件游标开始读取数据
 * @param p_file: struct File * :待读取的文件描述符
 * @param data: void *: 待读取的数据缓冲区
 * @param count: size_t :读取的数据量
 * @return 成功返回读取的数据量，失败返回-1
*/
int32_t file_read(struct File *p_file, void *data, size_t count);

#endif