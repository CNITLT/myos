#include "file.h"
#include "thread.h"
#include "bitmap.h"
#include "debug.h"
#include "fs.h"
#include "inode.h"
#include "dir.h"

struct File g_file_table[MAX_FD_SIZE];

/*
    @brief 从全局的文件描述符数组里获取一个空闲位, 失败返回-1
    @return uint32_t 可用的数组下标，识别则为-1
*/
int32_t get_free_file_slot_in_g_table() {
    for(int i = USED_FD_START_INDEX; i < MAX_FD_SIZE; i++) {
        if (g_file_table[i].p_fd_inode == NULL) {
            return i;
        }
    }
    return -1;
}

int32_t pcb_fd_install(int32_t globa_fd_index) {
    struct task_struct *pcb = get_current_pcb();
     for(int i = USED_FD_START_INDEX; i < MAX_FD_SIZE; i++) {
        if (pcb->fd_table[i] == -1) {
            pcb->fd_table[i] = globa_fd_index;
            return i;
        }
    }
    return -1;
}

int32_t inode_bitmap_alloc(struct Partition *p_part) {
    int32_t bit_index = bitmap_find_range(&p_part->inode_bitmap, 1);
    if (bit_index != BITMAP_RANGE_NOTFOUND) {
        bitmap_set(&p_part->inode_bitmap, bit_index, BIT_STATE_USE);
    }
    return bit_index;
}

void inode_bitmap_free(struct Partition *p_part, int32_t inode_no) {
    bitmap_set(&p_part->inode_bitmap, inode_no, BIT_STATE_UNUSE);
}

int32_t block_bitmap_alloc(struct Partition *p_part) {
    int32_t bit_index = bitmap_find_range(&p_part->block_bitmap, 1);
    if (bit_index == BITMAP_RANGE_NOTFOUND) {
        return bit_index;
    }
    // 此处返回的是扇区地址
    bitmap_set(&p_part->block_bitmap, bit_index, BIT_STATE_USE);
    return p_part->super_block->data_area_lba_base + bit_index;
}

void block_bitmap_free(struct Partition *p_part, int32_t block_lba) {
    int32_t bit_index = block_lba - p_part->super_block->data_area_lba_base;
    assert(bit_index != -1);
    bitmap_set(&p_part->block_bitmap, bit_index, BIT_STATE_UNUSE);
}

void bitmap_sync(struct Partition *p_part, int32_t bit_index, Bitmap_type bitmap_type) {
    // 相对于位图所在磁盘内的位图起点扇区的偏移
    uint32_t offset_sector = bit_index / BITS_PER_SECTOR;
    // 先对于内存位图的字节偏移
    uint32_t offset_byte = offset_sector * BLOCK_SIZE;
    uint32_t sector_lba;
    // bitmap内对应的一小块扇区的起点地址
    uint8_t *sector_bits;

    if (bitmap_type == Bitmap_type_inode) {
        sector_lba = p_part->super_block->inode_bitmap_lba + offset_sector;
        sector_bits = p_part->inode_bitmap.bits + offset_byte;
    } else if (bitmap_type == Bitmap_type_block) {
        sector_lba = p_part->super_block->block_bitmap_lba + offset_sector;
        sector_bits = p_part->block_bitmap.bits + offset_byte;
    }
    ide_write(p_part->p_disk, sector_lba, sector_bits, 1);
}

int32_t file_open(uint32_t inode_no, uint8_t flag) {
    int32_t fd_index = get_free_file_slot_in_g_table();
    if (fd_index == -1) {
        printf("file_open get_free_file_slot_in_g_table faild\n");
        return -1;
    }

    g_file_table[fd_index].p_fd_inode = inode_open(g_current_part, inode_no);
    g_file_table[fd_index].fd_pos = 0;
    g_file_table[fd_index].fd_flag = flag;

    int pcb_fd_index = pcb_fd_install(fd_index);
    if (pcb_fd_index == -1) {
        inode_close(g_file_table[fd_index].p_fd_inode);
        g_file_table[fd_index].p_fd_inode = NULL;
        printf("file_open pcb_fd_install faild\n");
        return -1;
    }

    bool *write_deny = &(g_file_table[fd_index].p_fd_inode->write_deny);

    if (flag & O_WR_ONLY || (flag & O_RDWR)) {
        interrupt_state old_intr_state = close_interrupt();
        // 这里设计上是只能有一个进程以写模式打开
        if (!(*write_deny)) {
            *write_deny = true;
            set_interrupt_state(old_intr_state);
        } else {
            set_interrupt_state(old_intr_state);
            printf("file_open write_deny set true faild\n");
            return -1;
        }
    }
    return pcb_fd_index;
}

int32_t file_close(struct File *p_file) {
    if (!p_file) {
        return -1;
    }
    p_file->p_fd_inode->write_deny = false;
    inode_close(p_file->p_fd_inode);
    p_file->p_fd_inode = NULL;
    return 0;
}

int32_t file_write(struct File *p_file, void *buff, size_t count) {
    if (p_file->p_fd_inode->i_size + count > MAX_FILE_CONTENT_SIZE) {
        // 超出写入范围了
        printf("file_write exceed max content size\n");
        return -1;
    }
    Byte *buff = sys_malloc(2 * BLOCK_SIZE);
    if (!buff) {
        printf("file_write malloc buff faild\n");
        return -1;
    }
    uint32_t *p_all_block_lba = NULL;
    uint32_t all_block_lba_count = 0;
    get_inode_all_block_lba(g_current_part, p_file->p_fd_inode, &p_all_block_lba, &all_block_lba_count);
    int32_t write_count = 0; // 写入量

    // 未写入数据的指针
    Byte *next = (Byte *)buff;
    // 还剩下多少未写入
    size_t next_count = count;
    
    while(write_count < count) {
        // 存在的会就是读取，不存在的会先分配然后返回
        int32_t block_lba = alloc_inode_all_block(g_current_part, p_file->p_fd_inode, p_all_block_lba, all_block_lba_count, all_block_write_index);
        if (block_lba == -1) {
            printf("file_write alloc_inode_all_block faild\n");
            return write_count;
        }
        // 操作的相关偏移
        int32_t all_block_write_index = p_file->p_fd_inode->i_size / BLOCK_SIZE;
        int32_t index_in_sector = p_file->p_fd_inode->i_size % BLOCK_SIZE;
        // 当前可写的剩余容量
        int32_t free_size_in_sector = BLOCK_SIZE - index_in_sector;
        // 本次实际写入的大小
        int32_t write_size_in_once = MIN(free_size_in_sector, next_count);
        ide_read(g_current_part->p_disk, block_lba, buff, 1);
        memcpy(((Byte *)buff + index_in_sector), next, write_size_in_once);
        ide_write(g_current_part->p_disk, block_lba, buff, 1);

        // 更新下数据
        p_file->p_fd_inode->i_size += write_size_in_once;
        next += write_size_in_once;
        next_count -= write_size_in_once;
        write_count += write_size_in_once;
        // 同步下inode
        memset(buff, 0, 2 * BLOCK_SIZE);
        inode_sync(g_current_part, p_file->p_fd_inode, buff);
    }

    sys_free(p_all_block_lba);
    sys_free(buff);
    return write_count;
}