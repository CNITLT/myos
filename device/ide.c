#include "ide.h"
#include "debug.h"
#include "io.h"
#include "timer.h"
#include "interrupt.h"
#include "thread.h"
#include "string.h"
#include "memory.h"
#include "syscall.h"
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
struct list g_partition_list;	 // 分区队列,只链接了能写数据的分区。启动分区就没放里面

void ide_init(){
    uint8_t disk_count = *((uint8_t*)(0x475));//这个地址放着由BIOS检测到的磁盘个数;

    printf("disk_count:%d\n", disk_count);
    list_init(&g_partition_list);

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

    //初始化磁盘相关数据结构
    for(int i = 0; i < g_channel_count; i++){
        g_ide_channels[i].devices[0].p_ide_channel = &g_ide_channels[i];
        g_ide_channels[i].devices[0].dev_number = MASTER_DISK_DEV_NUMBER;
        g_ide_channels[i].devices[1].p_ide_channel = &g_ide_channels[i]; 
        g_ide_channels[i].devices[1].dev_number = SLAVE_DISK_DEV_NUMBER; 
    } 

    int disk_init_done = 0;
    for(int i = 0; i < g_channel_count; i++){
        for(int j = 0; j < DISKS_PER_IDE_CHANNEL; j++){
            struct Disk* p_disk = &g_ide_channels[i].devices[j];
            p_disk->exist_flag = ide_identify(p_disk, NULL);
            if(p_disk->exist_flag){
                memset(p_disk->name, 0, sizeof(p_disk->name));
                p_disk->name[0] = 's';
                p_disk->name[1] = 'd';
                p_disk->name[2] = 'a' + disk_init_done;
                disk_partition_scan(p_disk);
                disk_init_done++;
            }
        }
    }  
}


void select_disk(struct Disk* p_disk){
    struct Ide_channel* p_ide_channel = p_disk->p_ide_channel; 
    outb(ide_reg_dev(p_ide_channel), 
    IDE_DEV_BIT_FIXED | IDE_DEV_LBA_MODE 
    | ((p_disk->dev_number == SLAVE_DISK_DEV_NUMBER)?IDE_DEV_SLAVE_DISK:IDE_DEV_MASTER_DISK));
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

void send_disk_operator_cmd_without_intr(struct Disk* p_disk, uint8_t cmd){
    struct Ide_channel* p_ide_channel = p_disk->p_ide_channel;
    p_ide_channel->expecting_intr_flag = false;
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
    uint32_t time_limit = 30 * 1000; //等30秒
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
    assert(p_disk->exist_flag);
    struct Ide_channel* p_ide_channel = p_disk->p_ide_channel;

    lock(&p_ide_channel->lock);
    uint32_t single_op_sector_count = 1; //单次操作的扇区数，不能超256
    uint32_t sector_done = 0;//已经完成的扇区数
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
    assert(p_disk->exist_flag);
    struct Ide_channel* p_ide_channel = p_disk->p_ide_channel;

    lock(&p_ide_channel->lock);
    uint32_t single_op_sector_count = 1; //单次操作的扇区数，不能超256
    uint32_t sector_done = 0;//已经完成的扇区数
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

    //debug("disk_interrupt_func\n");
    if(p_ide_channel->expecting_intr_flag){
        //说明操作完成
        //读取status让磁盘知道中断已经被处理
        //发送reset命令或者写入新命令也可以让磁盘知道中断被处理
        p_ide_channel->expecting_intr_flag = false;
        //唤醒等待的线程
        //debug("disk_interrupt_func before  semaphore_add\n");
        semaphore_add(&p_ide_channel->disk_done);
        inb(ide_reg_status(p_ide_channel));
    }
    //debug("disk_interrupt_func quit\n");
}

bool ide_identify(struct Disk* p_disk, void* buff){
    char disk_info[512];
    memset(disk_info,0,sizeof(disk_info));
    struct Ide_channel* p_ide_channel = p_disk->p_ide_channel;
    
    lock(&p_ide_channel->lock);
    select_disk(p_disk);
    /*
    //如果对应的磁盘不存在会因为无法触发中断导致卡死
    send_disk_operator_cmd(p_disk, IDE_CMD_IDENTIFY);
    uint8_t status = inb(ide_reg_status(p_ide_channel));
    printf("ide_identify send cmd read status:%d\n", status);
    debug("ide_identify befort  semaphore_sub\n");
    semaphore_sub(&p_disk->p_ide_channel->disk_done);
    debug("ide_identify after  semaphore_sub\n");
    */
    send_disk_operator_cmd_without_intr(p_disk, IDE_CMD_IDENTIFY);

    sleep_ms(10);
    if(!busy_wait_disk(p_disk)){
        uint8_t status = inb(ide_reg_status(p_ide_channel));
        if(0 == status){
            // 测了下没磁盘的时候这个值是0
            return false;
        }
        //其他情况就认为是磁盘出错
        PANIC("ide_identify faild! \n");
    }

    read_from_disk(p_disk, disk_info, 1);
    if(buff){
        memcpy(buff, disk_info, sizeof(disk_info));
    }
    unlock(&p_ide_channel->lock); 
    //这里缺少检测硬盘是否存在的代码
    char disk_sn[21] ;//序列号
    char disk_type[41];//型号
    memset(disk_sn,0,sizeof(disk_sn));
    memset(disk_type,0,sizeof(disk_type));
    uint32_t disk_size_sector =  0; //硬盘可用扇区数

    //序列号信息偏移量是20， 长度是20
    memcpy(disk_sn, &disk_info[20], 20);
    //型号信息的偏移量是54，长度是40
    memcpy(disk_type, &disk_info[54], 40);
    // 可用扇区数的偏移量是120, 长度4
    disk_size_sector = *(uint32_t*)(disk_info + 120);
    printf("disk SN:%s\n", disk_sn);
    printf("disk type:%s\n", disk_type);
    printf("disk sector:%d\n", disk_size_sector);
    printf("disk capacity:%d MB \n", disk_size_sector*512 / 1024 / 1024);
    return true;
}



static struct list_node* pf_disk_part_info(struct list_node*node, int arg){
    struct Partition* p_part = elem2entry(struct Partition, part_tag, node);
    printf("name:%s start:%d size:%d\n", p_part->name,p_part->start_lba,p_part->size_sector);
    //打印用所以返回false
    return false;
}

void disk_partition_scan(struct Disk* p_disk){
    struct Boot_secotr * p_boot_secotr = sys_malloc(sizeof(struct Boot_secotr));
    debug("disk_partition_scan before ide_read\n");
    ide_read(p_disk, 0, p_boot_secotr,1);
    
    uint8_t* p_table_hex = p_boot_secotr->partition_table;
    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 16; j++){
            printf("%x ", p_table_hex[i*16 + j]);
        }
        printf("\n");
    }
    
    
   /*
   这里不处理太多额外情况，只处理以下
   在主分区表里的启动扇区
   主分区
   逻辑分区
   逻辑分区里如果有启动扇区那也不管，直接当普通分区用, 虽然这个项目里也不会有这种情况
   */
    //先处理主分区
    struct Partition_table_entry* p_table = &p_boot_secotr->partition_table;
    p_disk->used_paimary_parts_size = 0;
    struct Partition* ext_part = NULL;
    for(int i = 0; i < 4; i++){
        struct Partition_table_entry* p_entry = p_table + i;
        if(p_entry->boot_signature == BOOT_SIGNATURE
        || p_entry->system_signature != SYSTEM_SIGNATURE_EMPTY){
            //主分区下的启动分区
            int index = p_disk->used_paimary_parts_size++;
            struct Partition* p_part = &p_disk->primary_parts[index];
            memset(p_part,0,sizeof(*p_part));
            p_part->partition_table_entry = *p_entry;
            p_part->start_lba = p_entry->start_sector_lba;
            p_part->size_sector = p_entry->total_sector_num;
            p_part->p_disk = p_disk;
            //分区命名从1开始
            sprintf(p_part->name, "%s%d",p_disk->name, index+1);
            list_init(&p_part->opened_inodes);

            if(p_entry->system_signature == SYSTEM_SIGNATURE_EXTERN){
                if(ext_part){
                    PANIC("disk_partition_scan faild!\nError:mutil ext partition\n");
                }
                ext_part = p_part;
            }
            else{
                if(p_entry->boot_signature != BOOT_SIGNATURE){
                    //加入列表
                    list_push_back(&g_partition_list,&p_part->part_tag);
                }
            }
        }
    }

    //处理扩展分区
    //扩展分区内的EBR内的表项目只用前两个
    //第一个以当前子扩展分区的起点为偏移基准的数据分区
    //第二个以主分区的扩展分区的起点为偏移基准的下一个子扩展分区, 就是一个指针
    /*
    手动改了一个扩展分区表的数据，如果主分区划分了两个数据分区
    fdisk命令不认第二个数据分区，直接忽略
    这里就不处理例外了，直接第一个是数据分区，第二个是指针，如果不是这种结构就报错
    */
    p_disk->used_logic_parts_size = 0;
    if(ext_part){
        const int ext_part_start_lba = ext_part->start_lba;
        int next_ebr_lba = ext_part_start_lba;
        
        while(next_ebr_lba){
            ide_read(p_disk, next_ebr_lba, p_boot_secotr,1);
            struct Partition_table_entry* p_entry_part = &p_boot_secotr->partition_table[0];
            struct Partition_table_entry* p_entry_next = &p_boot_secotr->partition_table[1];
            
            //检测下格式是否满足
            if(p_entry_part->system_signature == SYSTEM_SIGNATURE_EMPTY
            || p_entry_part->system_signature == SYSTEM_SIGNATURE_EXTERN){
                PANIC("disk_partition_scan faild!\nError:unsupport ext partition format\n");
            }


            int index = p_disk->used_logic_parts_size++;
            struct Partition* p_part = &p_disk->logic_parts[index];
            memset(p_part,0,sizeof(*p_part));

            p_part->partition_table_entry = *p_entry_part;
            p_part->start_lba = p_entry_part->start_sector_lba + next_ebr_lba;
            p_part->size_sector = p_entry_part->total_sector_num;
            p_part->p_disk = p_disk;
            //逻辑分区命名从5开始
            sprintf(p_part->name, "%s%d",p_disk->name, index+5);
            list_init(&p_part->opened_inodes);
            //加入列表
            list_push_back(&g_partition_list,&p_part->part_tag);            
            
            /*
            uint8_t *p = (uint8_t *)p_entry_next;
            for(int i = 0; i < 16;i++){
                printf("%x ", p[i]);
            }
            printf("\n");
            */

            if(p_entry_next->system_signature == SYSTEM_SIGNATURE_EXTERN){
                next_ebr_lba = ext_part_start_lba + p_entry_next->start_sector_lba;
            }   
            else if(p_entry_next->system_signature == SYSTEM_SIGNATURE_EMPTY) {
                next_ebr_lba = 0;
            }
            else{
                PANIC("disk_partition_scan faild!\nError:unsupport ext partition format\n");
            }
        }
        
    }
    sys_free(p_boot_secotr);
    //打印一下列表
    list_traversal(&g_partition_list,pf_disk_part_info,0) ;
}