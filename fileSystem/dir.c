#include "dir.h"
#include "ide.h"
#include "memory.h"
#include "debug.h"
#include "string.h"
#include "fs.h"
#include "file.h"
struct Dir g_root_dir;

void open_root_dir(struct Partition* p_part) {
    g_root_dir.p_inode = inode_open(p_part, p_part->super_block->root_inode_no);
    g_root_dir.dir_pos = 0;
}

struct Dir *dir_open(struct Partition* p_part, uint32_t inode_no) {
    struct Dir * p_dir = (struct Dir *)sys_malloc(sizeof(struct Dir));
    p_dir->p_inode = inode_open(p_part, inode_no);
    p_dir->dir_pos = 0;
    return p_dir;
}

void get_dir_all_block_lba(struct Partition* p_part, struct Dir *p_dir, uint32_t **p_all_block_lba_ret, uint32_t *p_all_block_lba_count_ret) {
 assert(BLOCK_SIZE % sizeof(uint32_t) == 0);
    // 对p_all_block_lba 的初始化感觉效率有点低，但无所谓了
    uint32_t all_block_count = I_NODE_LAYER0_BLCOK_SIZE + I_NODE_LAYER1_BLOCK_SIZE * BLOCK_SIZE / sizeof(uint32_t);
    uint32_t *p_all_block_lba = (uint32_t  *)sys_malloc(all_block_count * sizeof(uint32_t));
    uint32_t *p_block_iter = p_all_block_lba;
    for (int i = 0; i < I_NODE_LAYER0_BLCOK_SIZE; i++) {
        *p_block_iter = p_dir->p_inode->i_sectors[i];
        p_block_iter++;
    }
    Byte *buff = (Byte *)sys_malloc(BLOCK_SIZE);
    for (int i = I_NODE_LAYER0_BLCOK_SIZE; i < I_NODE_LAYER0_BLCOK_SIZE + I_NODE_LAYER1_BLOCK_SIZE; i++) {
        int j = I_NODE_LAYER0_SIZE_PER_LAYER1;
        if (p_dir->p_inode->i_sectors[i] == 0)  {
            while(j--) {
                *p_block_iter = 0;
                p_block_iter++;
            }
        } else {
            ide_read(p_part->p_disk, p_dir->p_inode->i_sectors[i], buff, 1);
            uint32_t *p_block_iter_src = buff;
            while(j--) {
                *p_block_iter = *p_block_iter_src;
                p_block_iter++;
                p_block_iter_src++;
            }
        }
    }
    *p_all_block_lba_ret = p_all_block_lba;
    *p_all_block_lba_count_ret = all_block_count;
    sys_free(buff);
}


bool search_dir_entry(struct Partition* p_part, struct Dir *p_dir, char *entry_name, struct Dir_entry* p_dir_entry) {  
    uint32_t all_block_count;
    uint32_t *p_all_block_lba;
    get_dir_all_block_lba(p_part, p_dir, &p_all_block_lba, &all_block_count);
    Byte *buff = (Byte *)sys_malloc(BLOCK_SIZE);
    // 保证项一定是在一个扇区里面，写目录的时候保证扇区末尾最后一定空间不足够赛下的时候就新开一个存
    for (int i = 0; i < all_block_count; i++) {
        // 因为从中间删除的话，中间是有可能有空洞的
        if (p_all_block_lba[i] == 0) {
            continue;
        }
        ide_read(p_part->p_disk, p_all_block_lba[i], buff, 1);
        struct Dir_entry*p_dir_entry_iter = buff;
        for (int j = 0; j < SECTOR_SIZE_BYTE / sizeof(struct Dir_entry); j++) {
            if (!strcmp(p_dir_entry_iter->fileName, entry_name) && p_dir_entry_iter->f_type != FT_UNKNOWN) {
                // 找到了就直接返回
                memcpy(p_dir_entry, p_dir_entry_iter, sizeof(struct Dir_entry));
                sys_free(buff);
                sys_free(p_all_block_lba);
                return true;
            }
            p_dir_entry_iter++;
        }
    }

    sys_free(buff);
    sys_free(p_all_block_lba);
    return false;
}


void dir_close(struct Dir * p_dir) {
    if (p_dir == &g_root_dir) {
        return;
    }
    inode_close(p_dir->p_inode);
    sys_free(p_dir);
}

void init_dir_entry(struct Dir_entry *p_dir_entry, char *fileName, uint32_t inode_no, File_types f_type) {
    assert(strlen(fileName) <= MAX_FILE_NAME_LENGTH);
    memcpy(p_dir_entry->fileName, fileName, strlen(fileName));
    p_dir_entry->i_no = inode_no;
    p_dir_entry->f_type = f_type;
}


bool sync_dir_entry(struct Partition* p_part, struct Dir* p_dir, struct Dir_entry *p_dir_entry, void *io_buff) {
    uint32_t all_block_count;
    uint32_t *p_all_block_lba;
    get_dir_all_block_lba(p_part, p_dir, &p_all_block_lba, &all_block_count);
    uint32_t dir_size = p_dir->p_inode->i_size;
    uint32_t dir_entry_size = p_part->super_block->dir_entry_size;
    bool need_free_buff = false;
    Byte *buff = io_buff;
    if (!buff) {
        need_free_buff = true;
        buff = (Byte *)sys_malloc(BLOCK_SIZE);
    }

    // 保证项一定是在一个扇区里面，写目录的时候保证扇区末尾最后一定空间不足够赛下的时候就新开一个存
    for (int i = 0; i < all_block_count; i++) {
        if (p_all_block_lba[i] == 0) {
            // 先分配一个扇区
            int32_t block_lba = block_bitmap_alloc(p_part);
            if (block_lba == -1) {
                printf("sync_dir_entry block_lba false");
                return false;
            }
            // 然后同步磁盘
            int32_t block_bitmap_index = block_lba - p_part->super_block->data_area_lba_base;
            assert(block_bitmap_index != -1);
            bitmap_sync(p_part, block_bitmap_index, Bitmap_type_block);

            // 判断是直接块还是间接块
            if (i < I_NODE_LAYER0_BLCOK_SIZE) {
                // 直接块
                p_dir->p_inode->i_sectors[i] = block_lba;
                p_all_block_lba[i] = block_lba;
            } else if ((i - I_NODE_LAYER0_BLCOK_SIZE) % I_NODE_LAYER1_BLOCK_SIZE == 0) {
                // 间接块, 则还需要分配一个块
                int32_t block_lba2 = block_bitmap_alloc(p_part);
                if (block_lba2 == -1) {
                    printf("sync_dir_entry block_lba_layer1 false");
                    // 此时撤回之前的修改与同步
                    block_bitmap_free(p_part, block_lba);
                    bitmap_sync(p_part, block_bitmap_index, Bitmap_type_block);
                    return false;
                }
                // 分配成功再同步一次磁盘
                int32_t block_bitmap_index2 = block_lba2 - p_part->super_block->data_area_lba_base;
                assert(block_bitmap_index2 != -1);
                bitmap_sync(p_part, block_bitmap_index2, Bitmap_type_block); 
                
                int32_t layer1_index = I_NODE_LAYER0_BLCOK_SIZE + ((i - I_NODE_LAYER0_BLCOK_SIZE) / I_NODE_LAYER1_BLOCK_SIZE);
                int32_t all_index = I_NODE_LAYER0_BLCOK_SIZE + (i - I_NODE_LAYER1_BLOCK_SIZE) * I_NODE_LAYER0_SIZE_PER_LAYER1;
                p_dir->p_inode->i_sectors[layer1_index] = block_lba;
                p_all_block_lba[all_index] = block_lba2;
                // 写入一级列表
                ide_write(p_part->p_disk, block_lba, p_all_block_lba + all_index, 1);

            } else {
                // 间距块里的直接块
                p_all_block_lba[i] = block_lba;
                ide_write(p_part->p_disk, block_lba, p_all_block_lba + I_NODE_LAYER0_BLCOK_SIZE + (((i - I_NODE_LAYER0_BLCOK_SIZE) / I_NODE_LAYER0_SIZE_PER_LAYER1) * I_NODE_LAYER0_SIZE_PER_LAYER1), 1);
            }
            // 初始化刚刚分配的块内容
            memset(buff, 0, BLOCK_SIZE);
            memcpy(buff, p_dir_entry, dir_entry_size);
            ide_write(p_part->p_disk, p_all_block_lba[i], buff, 1);
            p_dir->p_inode->i_size += dir_entry_size;
            // 同步inode信息
            inode_sync(p_part, p_dir->p_inode, NULL);
            return true;
        } else {
            // 块已经存在，遍历找空位
            ide_read(p_part->p_disk, p_all_block_lba[i], buff, 1);
            struct Dir_entry *p_dir_entry_iter = buff;
            while ((uint32_t)p_dir_entry_iter < ((uint32_t)(buff + BLOCK_SIZE))) {
                if (p_dir_entry_iter->f_type == FT_UNKNOWN) {
                    memcpy(p_dir_entry_iter, p_dir_entry, dir_entry_size);
                    ide_write(p_part->p_disk, p_all_block_lba[i], buff, 1);
                    p_dir->p_inode->i_size += dir_entry_size;
                    // 同步inode信息
                    inode_sync(p_part, p_dir->p_inode, NULL);
                    return true;
                }
                p_dir_entry_iter++;
            }
            
        }       
    }
    if (need_free_buff) {
        sys_free(buff);
    }
    sys_free(p_all_block_lba);
    return false;
}