#include "ide.h"

#define ide_reg_data(p_channel) (p_channel->port_base + 0)
#define ide_reg_error(p_channel) (p_channel->port_base + 1)
#define ide_reg_sector_count(p_channel) (p_channel->port_base + 2)
#define ide_reg_lba_low(p_channel) (p_channel->port_base + 3)
#define ide_reg_lba_middle(p_channel) (p_channel->port_base + 4)
#define ide_reg_lba_high(p_channel) (p_channel->port_base + 5)
#define ide_reg_dev(p_channel) (p_channel->port_base + 6)
//读模式就是状态寄存器
#define ide_reg_status(p_channel) (p_channel->port_base + 7)
//写模式就是命令寄存器
#define ide_reg_cmd(p_channel) (ide_reg_status(p_channel)) 
//这个寄存器同上，分读写两种
#define ide_reg_alt_status(p_channel) (p_channel->port_base + 0x206)
#define ide_reg_ctl(p_channel) (ide_reg_alt_status(p_channel))

//主要是这几个
#define IDE_ALT_STATUS_BIT_BSY 0x80  //硬盘繁忙
#define IDE_ALT_STATUS_BIT_DRQ 0x8 //硬盘准备好数据交换
#define IDE_ALT_STATUS_BIT_ERR 0x1 //有错误发生
//这个好像也不太重要
#define IDE_ALT_STATUS_BIT_RDY 0x40 //硬盘正在以正确的速度旋转

//下面这些状态不是太重要, 有些资料说已经废弃了
#define IDE_ALT_STATUS_BIT_WFT 0x20 //有写入错误
#define IDE_ALT_STATUS_BIT_SKC 0x10 //寻道完成，磁头就位
#define IDE_ALT_STATUS_BIT_COR 0x4 //为1代表磁盘进行了数据纠正
#define IDE_ALT_STATUS_BIT_IDX 0x2 //为1代表磁盘正确找到起始扇区，为0表示没找到操作的起始扇区

/*device寄存器*/
#define IDE_DEV_BIT_FIXED 0xa0 //有这两个位要固定为1
#define IDE_DEV_BIT_MODE 0x40 //操作模式
#define IDE_DEV_BIT_DISK 0x10 //操作磁盘
#define IDE_DEV_BITS_LBA24_27 0x0F //剩下的LBA位置24-27

#define IDE_DEV_CHS_MODE 0x00 //索引为6的位置为0代表用CHS的操作方式
#define IDE_DEV_LBA_MODE 0x40 //索引为6的位置为1代表用LBA的操作方式
#define IDE_DEV_MASTER_DISK 0x00 //索引为4的位置， 为0代表为主盘
#define IDE_DEV_SLAVE_DISK 0x10 //索引为4的位置， 为1代表为从盘

//操作命令
#define IDE_CMD_IDENTIFY 0xEC
#define IDE_CMD_READ_SECTOR 0x20
#define IDE_CMD_WRITE_SECTOR 0x30

#define MAX_DISK_SIZE_MB 80 //最大支持磁盘容量
#define MAX_LBA ((MAX_DISK_SIZE_MB*1024*1024/SECTOR_SIZE_BYTE) - 1) //磁盘容量的最大LBA值

uint8_t g_channel_count; //当前通道数
struct Ide_channel g_ide_channels[MAX_IDE_CHANNEL_COUNT]; //最大支持2个IDE通道 4个硬盘

void ide_init(){
    uint8_t disk_count = *((uint8_t*)(0x475));//这个地址放着由BIOS检测到的磁盘个数;
    g_channel_count = DIV_ROUND_UP(disk_count, 2);
    g_ide_channels[0].port_base = 0x1f0;
    g_ide_channels[0].interrupt_number = 0x20 + 14;
    g_ide_channels[0].expecting_intr_flag = false;
    mutex_init(&(g_ide_channels[0].lock));
    semaphore_init(&(g_ide_channels[0].wait_disk_process), 0);
    g_ide_channels[1].port_base = 0x170;
    g_ide_channels[1].interrupt_number = 0x20 + 15;
    g_ide_channels[1].expecting_intr_flag = false;
    mutex_init(&(g_ide_channels[1].lock));
    semaphore_init(&(g_ide_channels[1].wait_disk_process), 0);
}