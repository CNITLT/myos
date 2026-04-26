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

bool search_dir_entry(struct Partition* p_part, struct Dir *p_dir, char *entry_name, struct Dir_entry* p_dir_entry) {  
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


    sys_free(buff);
    sys_free(p_all_block_lba);
}