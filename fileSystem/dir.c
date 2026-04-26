#include "dir.h"
#include "ide.h"
#include "memory.h"
#include "debug.h"
#include "string.h"
struct Dir g_root_dir;

void open_root_dir(struct Partition* p_part) {
    g_root_dir.inode = inode_open(p_part, p_part->super_block->root_inode_no);
    g_root_dir.dir_pos = 0;
}

struct Dir *dir_open(struct Partition* p_part, uint32_t inode_no) {
    struct Dir * p_dir = (struct Dir *)sys_malloc(sizeof(struct Dir));
    p_dir->inode = inode_open(p_part, inode_no);
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
        *p_block_iter = p_dir->inode->i_sectors[i];
        p_block_iter++;
    }
    Byte *buff = (Byte *)sys_malloc(BLOCK_SIZE);
    for (int i = I_NODE_LAYER0_BLCOK_SIZE; i < I_NODE_LAYER0_BLCOK_SIZE + I_NODE_LAYER1_BLOCK_SIZE; i++) {
        int j = BLOCK_SIZE / sizeof(uint32_t);
        if (p_dir->inode->i_sectors[i] == 0)  {
            while(j--) {
                *p_block_iter = 0;
                p_block_iter++;
            }
        } else {
            ide_read(p_part->p_disk, p_dir->inode->i_sectors[i], buff, 1);
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
    inode_close(&p_dir->inode);
    sys_free(p_dir);
}

void create_dir_entry(struct Dir_entry *p_dir_entry, char *fileName, uint32_t inode_no, File_types f_type) {
    assert(strlen(fileName) <= MAX_FILE_NAME_LENGTH);
    memcpy(p_dir_entry->fileName, fileName, strlen(fileName));
    p_dir_entry->i_no = inode_no;
    p_dir_entry->f_type = f_type;
}