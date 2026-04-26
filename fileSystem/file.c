#include "file.h"
#include "thread.h"
#include "bitmap.h"
static struct File g_file_table[MAX_FD_SIZE];

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


int32_t block_bitmap_alloc(struct Partition *p_part) {
    int32_t bit_index = bitmap_find_range(&p_part->block_bitmap, 1);
    if (bit_index == BITMAP_RANGE_NOTFOUND) {
        return bit_index;
    }
    // 此处返回的是扇区地址
    bitmap_set(&p_part->block_bitmap, bit_index, BIT_STATE_USE);
    return p_part->super_block->data_area_lba_base + bit_index;
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