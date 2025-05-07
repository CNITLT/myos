#ifndef __DEVICE_IDE_H 
#define __DEVICE_IDE_H
#include "stdint.h"
#include "list.h"
#include "bitmap.h"
#include "mutex.h"

#define PARTITION_NAME_SIZE 8
#define DISK_NAME_SIZE 8
#define IDE_CHANNEL_NAME_SIZE 8
#define PRIMARY_PART_SIZE 4
#define LOGIC_PART_SIZE 16
#define DISKS_PER_IDE_CHANNEL 2
#define SECTOR_SIZE_BYTE 512
#define MAX_IDE_CHANNEL_COUNT 2
#define MASTER_DISK_DEV_NUMBER 0
#define SLAVE_DISK_DEV_NUMBER 1

#define BOOT_SIGNATURE 0x80
#define NON_BOOT_SIGNATURE 0x0

#define SYSTEM_SIGNATURE_NULL 0 //空表示无分区
#define SYSTEM_SIGNATURE_EXTERN 0x5 //扩展分区
#define BOOT_SECTOR_MAGIC_NUMBER 0xAA55 //小端是反着来的
/*
分区结构
*/
struct Partition{
    uint32_t start_lba; //起始扇区
    uint32_t size_sector; //扇区数
    struct Disk* p_disk; //所属磁盘
    struct list_node part_tag; //用于队列的标记
    char name[PARTITION_NAME_SIZE]; //分区名字
    struct Super_block* super_block;//本分区的超级块
    struct bitmap block_bitmap; //块位图
    struct bitmap inode_bitmap; //inode位图
    struct list opened_inodes; //打开的inode节点队列
};

/*分区表项*/
struct Partition_table_entry {
    uint8_t boot_signature;//引导标志 一般就两个值0x80和0, 0x80表示是有引导代码的 ,0为非活动分区
    uint8_t start_head;//CHS寻址方式，起始磁头
    uint8_t start_sector;//起始扇区，本字节低六位
    uint8_t start_cylinder;//起始磁道(柱面)，startSector高二位和本字节
    uint8_t system_signature;//文件系统类型标志
    uint8_t end_head;//终止磁头
    uint8_t end_sector;//终止扇区
    uint8_t end_cylinder;//终止磁道
    uint32_t start_sectorNo;//LBA寻址，起始扇区号
    uint32_t total_sectorsNum;//该分区扇区总数
} __attribute__ ((packed));

//初始扇区对应的结构
struct Boot_secotr{
    uint8_t other[446]; //如果是引导扇区就是对应的代码
    struct Partition_table_entry partition_table[4] ;//4个表项目
    uint16_t magic_number; //0x55 0xaa是有效扇区的签名
} __attribute__ ((packed));


struct Disk{
    char name[DISK_NAME_SIZE]; //磁盘名
    struct Ide_channel* p_ide_channel; //磁盘所属的ide通道
    uint8_t dev_number; //主盘是0， 从盘是1
    struct Partition primary_parts[PRIMARY_PART_SIZE]; //主分区 最多4个
    struct Partition logic_parts[LOGIC_PART_SIZE]; //逻辑分区 有最多支持数，由宏设定
    bool exist_flag; //磁盘是否存在
};


struct Ide_channel{
    char name[IDE_CHANNEL_NAME_SIZE]; //通道名
    //通道起始端口，后续部分端口是这个端口计算
    // 两IDE通道的端口分配如下
    // 通道1 主通道 命令块端口 0X1F0~0X1F7 控制块端口0X3F6 即在0x1F0上分别加0~7 和 0X206
    // 通道2 主通道 命令块端口 0X170~0X177 控制块端口0X376 即在0x170上分别加0~7 和 0X206
    uint16_t port_base; 
    uint8_t interrupt_number; //中断号
    struct mutex lock; //锁
    bool expecting_intr_flag; //表示是否在等待中断
    struct semaphore disk_done; //用于等待磁盘操作完成的程序对应的信号量
    struct Disk devices[DISKS_PER_IDE_CHANNEL]; //通道上的磁盘
};

/*
@brief 初始化硬盘数据结构
*/
void ide_init();

/*
@brief 选择磁盘，用于在不需要lba的命令下使用
@param p_disk: struct Disk* : 磁盘数据结构地址
*/
void select_disk(struct Disk* p_disk);

/*
@brief 写入起始扇区和操作扇区数并选择主盘或从盘
@param p_disk: struct Disk* : 磁盘数据结构地址
@param lba_addr: uint32_t :扇区起始地址 
@param sector_count : uint32_t :要操作的扇区数, 单次最多是1-256
*/
void select_disk_and_sector(struct Disk* p_disk, uint32_t lba_addr, uint32_t sector_count);

/*
@brief 向磁盘发送命令
@param p_disk: struct Disk* : 磁盘数据结构地址
@param cmd: uint8_t :操作命令
*/
void send_disk_operator_cmd(struct Disk* p_disk, uint8_t cmd);

/*
@brief 向磁盘发送命令, 但不需要中断，需要使用轮询处理, 用于identify命令在无磁盘时候不会有中断的情况
@param p_disk: struct Disk* : 磁盘数据结构地址
@param cmd: uint8_t :操作命令
*/
void send_disk_operator_cmd_without_intr(struct Disk* p_disk, uint8_t cmd);


/*
@brief 从准备好数据的磁盘里读取数据到buff
@param p_disk: struct Disk* : 磁盘数据结构地址
@param buff: void* : 数据存储地址 
@param sector_count : uint32_t :要操作的扇区数, 单次最多是1-256
*/
void read_from_disk(struct Disk* p_disk, void* buff, uint32_t sector_count);


/*
@brief 将buff写到磁盘
@param p_disk: struct Disk* : 磁盘数据结构地址
@param buff: void* : 待写入数据地址 
@param sector_count : uint32_t :要操作的扇区数, 单次最多是1-256
*/
void write_to_disk(struct Disk* p_disk, void* buff, uint32_t sector_count);

/*
@brief 等待30s， ATA手册说明，所有操作需要在31s内完成，这里取30s
@param p_disk: struct Disk* : 磁盘数据结构地址
@param bool: 为true表示可以进行后续操作, 返回false说明超时
*/
bool busy_wait_disk(struct Disk* p_disk);

/*
@brief 从磁盘内的指定位置读取sector_count个扇区的数据到buff内
@param p_disk: struct Disk* : 磁盘数据结构地址
@param lba_addr: uint32_t :扇区起始地址 
@param buff: void* : 数据存储地址 
@param sector_count : uint32_t :要操作的扇区数
*/
void ide_read(struct Disk* p_disk, uint32_t lba_addr, void* buff, uint32_t sector_count);


/*
@brief 将buff数据写入到磁盘内的指定位置起sector_count个扇区内
@param p_disk: struct Disk* : 磁盘数据结构地址
@param lba_addr: uint32_t :扇区起始地址 
@param buff: void* : 数据存储地址 
@param sector_count : uint32_t :要操作的扇区数
*/
void ide_write(struct Disk* p_disk, uint32_t lba_addr, void* buff, uint32_t sector_count);


/*
@brief 将identify 命令结果写入到buff内
@param p_disk: struct Disk* : 磁盘数据结构地址
@param buff: void* : 数据存储地址, 为null就只检测磁盘是否存在
@return bool: 若检测不到磁盘，返回false
*/
bool ide_identify(struct Disk* p_disk, void* buff);

/*
@brief 硬盘中断函数
*/
void disk_interrupt_func(void);



#endif 
