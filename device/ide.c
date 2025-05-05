#include "ide.h"
#include "debug.h"
#include "io.h"
#include "timer.h"
#include "interrupt.h"
#include "thread.h"
#define ide_reg_data(p_ide_channel) (p_ide_channel->port_base + 0)
#define ide_reg_error(p_ide_channel) (p_ide_channel->port_base + 1)
#define ide_reg_sector_count(p_ide_channel) (p_ide_channel->port_base + 2)
#define ide_reg_lba_low(p_ide_channel) (p_ide_channel->port_base + 3)
#define ide_reg_lba_middle(p_ide_channel) (p_ide_channel->port_base + 4)
#define ide_reg_lba_high(p_ide_channel) (p_ide_channel->port_base + 5)
#define ide_reg_dev(p_ide_channel) (p_ide_channel->port_base + 6)
//读模式就是状态寄存器
#define ide_reg_status(p_ide_channel) (p_ide_channel->port_base + 7)
//写模式就是命令寄存器
#define ide_reg_cmd(p_ide_channel) (ide_reg_status(p_ide_channel)) 
//这个寄存器同上，分读写两种
#define ide_reg_alt_status(p_ide_channel) (p_ide_channel->port_base + 0x206)
#define ide_reg_ctl(p_ide_channel) (ide_reg_alt_status(p_ide_channel))

//主要是这几个
#define IDE_STATUS_BIT_BSY 0x80  //硬盘繁忙
#define IDE_STATUS_BIT_DRQ 0x8 //硬盘准备好数据交换
#define IDE_STATUS_BIT_ERR 0x1 //有错误发生
//这个好像也不太重要
#define IDE_STATUS_BIT_RDY 0x40 //硬盘正在以正确的速度旋转

//下面这些状态不是太重要, 有些资料说已经废弃了
#define IDE_STATUS_BIT_WFT 0x20 //有写入错误
#define IDE_STATUS_BIT_SKC 0x10 //寻道完成，磁头就位
#define IDE_STATUS_BIT_COR 0x4 //为1代表磁盘进行了数据纠正
#define IDE_STATUS_BIT_IDX 0x2 //为1代表磁盘正确找到起始扇区，为0表示没找到操作的起始扇区

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
    semaphore_init(&(g_ide_channels[0].disk_done), 0);
    g_ide_channels[1].port_base = 0x170;
    g_ide_channels[1].interrupt_number = 0x20 + 15;
    g_ide_channels[1].expecting_intr_flag = false;
    mutex_init(&(g_ide_channels[1].lock));
    semaphore_init(&(g_ide_channels[1].disk_done), 0);

    //注册中断函数
    for(int i = 0; i < g_channel_count; i++){
        register_interrupt_func(g_ide_channels[i].interrupt_number, disk_interrupt_func);
    }

}


void select_disk_and_sector(struct Disk* p_disk, uint32_t lba_addr, uint32_t sector_count){
    assert(lba_addr + sector_count < MAX_LBA);
    assert(sector_count <= 256);
    struct Ide_channel* p_ide_channel = p_disk->p_ide_channel;
    /*
    写入扇区数,一次最多是256个
    */
    outb(ide_reg_sector_count(p_ide_channel), sector_count & 0xFF);
    //写入lba地址
    outb(ide_reg_lba_low(p_ide_channel), lba_addr & 0xFF);
    outb(ide_reg_lba_middle(p_ide_channel), (lba_addr >> 8)& 0xFF);
    outb(ide_reg_lba_high(p_ide_channel), (lba_addr >> 16)& 0xFF);
    //还有4个LBA地址位在dev寄存器里面
    //IDE_DEV_BIT_FIXED | IDE_DEV_LBA_MODE 基本是固定的，也不会用到CHS模式
    outb(ide_reg_dev(p_ide_channel), 
    IDE_DEV_BIT_FIXED | IDE_DEV_LBA_MODE 
    | ((p_disk->dev_number == SLAVE_DISK_DEV_NUMBER)?IDE_DEV_SLAVE_DISK:IDE_DEV_MASTER_DISK) 
    | (lba_addr >> 24) & 0xF);
}

void send_disk_operator_cmd(struct Disk* p_disk, uint8_t cmd){
    struct Ide_channel* p_ide_channel = p_disk->p_ide_channel;
    p_ide_channel->expecting_intr_flag = true;
    outb(ide_reg_cmd(p_ide_channel), cmd);
}

void read_from_disk(struct Disk* p_disk, void* buff, uint32_t sector_count){
    uint32_t size_word = sector_count * 512 / 2;
    insw(ide_reg_data(p_disk->p_ide_channel), buff, size_word);
}

void write_to_disk(struct Disk* p_disk, void* buff, uint32_t sector_count){
    uint32_t size_word = sector_count * 512 / 2;
    outsw(ide_reg_data(p_disk->p_ide_channel), buff, size_word);
}

bool busy_wait_disk(struct Disk* p_disk){
    struct Ide_channel* p_ide_channel = p_disk->p_ide_channel;
    uint32_t time_limit = 30 * 1000; //等30毫秒
    uint32_t sleep_time_ms = 10;
    uint32_t wait_time_ms = 0;
    while(wait_time_ms < time_limit){
        uint8_t status = inb(ide_reg_status(p_ide_channel));
        if(!(status & IDE_STATUS_BIT_BSY)){
            status = inb(ide_reg_status(p_ide_channel)); 
            return status & IDE_STATUS_BIT_DRQ;
        }
        sleep_ms(sleep_time_ms);
        wait_time_ms += sleep_time_ms;
    }
    return false;
}


void ide_read(struct Disk* p_disk, uint32_t lba_addr, void* buff, uint32_t sector_count){
    assert(lba_addr + sector_count < MAX_LBA);
    struct Ide_channel* p_ide_channel = p_disk->p_ide_channel;

    lock(&p_ide_channel->lock);
    uint32_t single_op_sector_count; //单次操作的扇区数，不能超256
    uint32_t sector_done;//已经完成的扇区数
    while(sector_done < sector_count){
        //计算单次操作的扇区数
        if(sector_done + 256 < sector_count){
            single_op_sector_count = 256;
        }
        else{
            single_op_sector_count = sector_count - sector_done;
        }
        //写入对应的lba地址 单次操作的扇区数 操作主从盘及命令
        select_disk_and_sector(p_disk, lba_addr + sector_done, single_op_sector_count);
        send_disk_operator_cmd(p_disk, IDE_CMD_READ_SECTOR);
        /*等待信号量*/
        semaphore_sub(&p_ide_channel->disk_done);
        
        //醒来后检测磁盘状态
        if(!busy_wait_disk(p_disk)){
            //若为false则出现问题，直接挂掉整个系统
            PANIC("disk read failed\n") ;
        }

        //开始读取数据
        read_from_disk(p_disk,(void *)((uint32_t)buff + (sector_done * 512)), single_op_sector_count);
        sector_done += single_op_sector_count;
    }
    unlock(&p_disk->p_ide_channel->lock);
}



void ide_write(struct Disk* p_disk, uint32_t lba_addr, void* buff, uint32_t sector_count){
    assert(lba_addr + sector_count < MAX_LBA);
    struct Ide_channel* p_ide_channel = p_disk->p_ide_channel;

    lock(&p_ide_channel->lock);
    uint32_t single_op_sector_count; //单次操作的扇区数，不能超256
    uint32_t sector_done;//已经完成的扇区数
    while(sector_done < sector_count){
        //计算单次操作的扇区数
        if(sector_done + 256 < sector_count){
            single_op_sector_count = 256;
        }
        else{
            single_op_sector_count = sector_count - sector_done;
        }
        //写入对应的lba地址 单次操作的扇区数 操作主从盘及命令
        select_disk_and_sector(p_disk, lba_addr + sector_done, single_op_sector_count);
        send_disk_operator_cmd(p_disk, IDE_CMD_WRITE_SECTOR);

        //等待磁盘能进行写入
        if(!busy_wait_disk(p_disk)){
            //若为false则出现问题，直接挂掉整个系统
            PANIC("disk write failed\n") ;
        }

        //开始读取数据
        write_to_disk(p_disk,(void *)((uint32_t)buff + (sector_done * 512)), single_op_sector_count);

        /*等待信号量*/
        semaphore_sub(&p_ide_channel->disk_done);
        sector_done += single_op_sector_count;
    }
    unlock(&p_disk->p_ide_channel->lock); 
}


void disk_interrupt_func(void){
    GET_P_INRE_STACK;
    uint32_t interrupt_number = p_intr_stack->interrupt_num;
    uint32_t ide_channel_number = interrupt_number - 0x20 - 14;
    struct Ide_channel* p_ide_channel = &g_ide_channels[ide_channel_number];
    assert(p_ide_channel->interrupt_number == interrupt_number);
    if(p_ide_channel->expecting_intr_flag){
        //说明操作完成
        //读取status让磁盘知道中断已经被处理
        //发送reset命令或者写入新命令也可以让磁盘知道中断被处理
        p_ide_channel->expecting_intr_flag = false;
        //唤醒等待的线程
        semaphore_add(&p_ide_channel->disk_done);
        inb(ide_reg_status(p_ide_channel));
    }
}