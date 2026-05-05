#include "dir.h"
#include "ide.h"
#include "memory.h"
#include "debug.h"
#include "string.h"
#include "fs.h"
#include "file.h"
struct Dir g_root_dir;

void open_root_dir(struct Partition* p_part) {
    // printf("debug open_root_dir p_part:%x name:%s root_inode_no:%d", p_part, p_part->name,  p_part->super_block->root_inode_no);
    // while(1){};
    g_root_dir.p_inode = inode_open(p_part, p_part->super_block->root_inode_no);
    g_root_dir.dir_pos = 0;
    g_root_dir.p_dir_entry = NULL;
    //printf("debug open_root_dir p_part:%x name:%s root_inode_no:%d g_root_dir.inode:0x%x\n", p_part, p_part->name,  p_part->super_block->root_inode_no, g_root_dir.p_inode); 
}

struct Dir *dir_open(struct Partition* p_part, uint32_t inode_no) {
    struct Dir * p_dir = (struct Dir *)sys_malloc(sizeof(struct Dir));
    assert(p_dir);
    p_dir->p_inode = inode_open(p_part, inode_no);
    p_dir->dir_pos = 0;
    p_dir->p_dir_entry = NULL;
    return p_dir;
}

void get_dir_all_block_lba(struct Partition* p_part, struct Dir *p_dir, uint32_t **p_all_block_lba_ret, uint32_t *p_all_block_lba_count_ret) {
    get_inode_all_block_lba(p_part, p_dir->p_inode, p_all_block_lba_ret, p_all_block_lba_count_ret);
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
    memset(p_dir_entry, 0, sizeof(struct Dir_entry));
    memcpy(p_dir_entry->fileName, fileName, strlen(fileName));
    p_dir_entry->i_no = inode_no;
    p_dir_entry->f_type = f_type;
    p_dir_entry->magic = DIR_ENTRY_MAGIC;
}

bool search_dir_entry_book_version(struct Partition* p_part, struct Dir *p_dir, char *entry_name, struct Dir_entry* p_dir_entry) {  
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
        memset(buff,0,BLOCK_SIZE);
        ide_read(p_part->p_disk, p_all_block_lba[i], buff, 1);
        struct Dir_entry*p_dir_entry_iter = (struct Dir_entry*)buff;
        while((uint32_t)p_dir_entry_iter < (uint32_t)(buff + BLOCK_SIZE)) {
            // printf("debug search_dir_entry iter:0x%x filename:%s target name:%s cmp:%d iter type:%d\n",p_dir_entry_iter, p_dir_entry_iter->fileName, entry_name, strcmp(p_dir_entry_iter->fileName, entry_name),  p_dir_entry_iter->f_type);
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

bool sync_dir_entry_book_version(struct Partition* p_part, struct Dir* p_dir, struct Dir_entry *p_dir_entry, void *io_buff) {
    uint32_t all_block_count;
    uint32_t *p_all_block_lba;
    get_dir_all_block_lba(p_part, p_dir, &p_all_block_lba, &all_block_count);
    uint32_t dir_size = p_dir->p_inode->i_size;
    uint32_t dir_entry_size = sizeof(struct Dir_entry);
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
                sys_free(p_all_block_lba);
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
                    sys_free(p_all_block_lba);
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
            //printf("debug sync_dir_entry if inode_sync part:%x inode:%x", p_part, p_dir->p_inode);
            inode_sync(p_part, p_dir->p_inode, NULL);
            sys_free(p_all_block_lba);
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
                    //printf("debug sync_dir_entry else  inode_sync part:%x inode:%x", p_part, p_dir->p_inode);
                    inode_sync(p_part, p_dir->p_inode, NULL);
                    sys_free(p_all_block_lba);
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

// 自己的魔改版本，目录项可跨扇区，紧凑版本
bool search_dir_entry(struct Partition* p_part, struct Dir *p_dir, char *entry_name, struct Dir_entry* p_dir_entry) {  
    assert(sizeof(struct Dir_entry) <= BLOCK_SIZE);
    Byte *buff = (Byte *)sys_malloc(BLOCK_SIZE);
    if (!buff) {
        printf("search_dir_entry malloc buff faild\n");
        return false;
    }
    uint32_t all_block_count;
    uint32_t *p_all_block_lba;
    get_dir_all_block_lba(p_part, p_dir, &p_all_block_lba, &all_block_count);
 
    const int32_t entry_count_in_block = BLOCK_SIZE / sizeof(struct Dir_entry);
    const int32_t max_used_read_size_in_once = entry_count_in_block * sizeof(struct Dir_entry);
    
    int pos = 0; // 当前读取的位置
    while(pos < p_dir->p_inode->i_size) {
        int32_t read_count = read_data_from_inode(p_part, p_dir->p_inode, p_all_block_lba, all_block_count, pos, buff, max_used_read_size_in_once);
        // printf("debug seach_dir_entry read p_part:0x%x dir_inode:0x%x pos:%d max_read:%d res:%d\n",p_part, p_dir->p_inode, pos, max_used_read_size_in_once, read_count);
        if (read_count == -1 || read_count == 0) {
            break;
        }
        // 开始遍历目录项
        struct Dir_entry *p_dir_entry_iter = (struct Dir_entry *)buff;
        while((uint32_t)p_dir_entry_iter < ((uint32_t)buff + read_count)) {
           // printf("debug seach_dir_entry p_dir_entry_iter name:%s target:%s cmp:%d iter type:%d magic:0x%x\n", p_dir_entry_iter->fileName, entry_name,strcmp(p_dir_entry_iter->fileName, entry_name), p_dir_entry_iter->f_type, p_dir_entry_iter->magic);
           if (!strcmp(p_dir_entry_iter->fileName, entry_name) && p_dir_entry_iter->f_type != FT_UNKNOWN && p_dir_entry_iter->magic == DIR_ENTRY_MAGIC) {
                // 找到了就直接返回
                memcpy(p_dir_entry, p_dir_entry_iter, sizeof(struct Dir_entry));
                sys_free(buff);
                sys_free(p_all_block_lba);
                return true;
            }
            p_dir_entry_iter++;
        }
        pos += read_count;
    }
    sys_free(buff);
    sys_free(p_all_block_lba);
    return false;
}

// 自己的魔改版本，目录项可跨扇区，紧凑版本
bool sync_dir_entry(struct Partition* p_part, struct Dir* p_dir, struct Dir_entry *p_dir_entry, void *io_buff) {
    assert(sizeof(struct Dir_entry) <= BLOCK_SIZE);
    Byte *buff = (Byte *)sys_malloc(BLOCK_SIZE);
    if (!buff) {
        printf("search_dir_entry malloc buff faild\n");
        return false;
    }
    uint32_t all_block_count;
    uint32_t *p_all_block_lba;
    get_dir_all_block_lba(p_part, p_dir, &p_all_block_lba, &all_block_count);
    const int32_t entry_count_in_block = BLOCK_SIZE / sizeof(struct Dir_entry);
    const int32_t max_used_read_size_in_once = entry_count_in_block * sizeof(struct Dir_entry);
    
    int pos = 0; // 当前读取的位置
    int32_t empty_dir_entry_pos = -1;
    while(pos < p_dir->p_inode->i_size) {
        int32_t read_count = read_data_from_inode(p_part, p_dir->p_inode, p_all_block_lba, all_block_count, pos, buff, max_used_read_size_in_once);
        if (read_count == -1 || read_count == 0) {
            break;
        }
        // 开始遍历目录项，查找空洞位置
        struct Dir_entry *p_dir_entry_iter = (struct Dir_entry *)buff;
        while((uint32_t)p_dir_entry_iter < ((uint32_t)buff + read_count)) {
           if (p_dir_entry_iter->f_type == FT_UNKNOWN && p_dir_entry_iter->magic == DIR_ENTRY_MAGIC) {
                empty_dir_entry_pos = pos + ((uint32_t)p_dir_entry_iter - (uint32_t)buff);
                break;
            }
            p_dir_entry_iter++;
        }
        pos += read_count;
    }
    if (empty_dir_entry_pos == -1) {
        empty_dir_entry_pos = p_dir->p_inode->i_size;
    }
    int write_count = write_data_to_inode(p_part, p_dir->p_inode, p_all_block_lba, all_block_count, empty_dir_entry_pos, p_dir_entry, sizeof(struct Dir_entry));
    sys_free(buff);
    sys_free(p_all_block_lba);
    if (write_count > 0 && empty_dir_entry_pos == p_dir->p_inode->i_size) { 
        //同步到磁盘
        p_dir->p_inode->i_size += write_count;
        inode_sync(p_part, p_dir->p_inode, NULL);
    }
    return write_count > 0;
}


bool delete_dir_entry(struct Partition* p_part, struct Dir* p_dir, struct Dir_entry *p_dir_entry, void *io_buff) {
    assert(sizeof(struct Dir_entry) <= BLOCK_SIZE);
    Byte *buff = (Byte *)sys_malloc(BLOCK_SIZE);
    if (!buff) {
        printf("search_dir_entry malloc buff faild\n");
        return false;
    }
    uint32_t all_block_count;
    uint32_t *p_all_block_lba;
    get_dir_all_block_lba(p_part, p_dir, &p_all_block_lba, &all_block_count);
    const int32_t entry_count_in_block = BLOCK_SIZE / sizeof(struct Dir_entry);
    const int32_t max_used_read_size_in_once = entry_count_in_block * sizeof(struct Dir_entry);
    
    int pos = 0; // 当前读取的位置
    int32_t delete_dir_entry_pos = -1;
    while(pos < p_dir->p_inode->i_size) {
        int32_t read_count = read_data_from_inode(p_part, p_dir->p_inode, p_all_block_lba, all_block_count, pos, buff, max_used_read_size_in_once);
        if (read_count == -1 || read_count == 0) {
            break;
        }
        // 开始遍历目录项，查找空洞位置
        struct Dir_entry *p_dir_entry_iter = (struct Dir_entry *)buff;
        while((uint32_t)p_dir_entry_iter < ((uint32_t)buff + read_count)) {
            // 目录空洞先跳过
           if (p_dir_entry_iter->f_type == FT_UNKNOWN || p_dir_entry_iter->magic != DIR_ENTRY_MAGIC) {
                p_dir_entry_iter++;
                continue;
            }
            if (!strcmp(p_dir_entry_iter->fileName, p_dir_entry->fileName) && p_dir_entry_iter->i_no == p_dir_entry->i_no) {
                delete_dir_entry_pos = pos + ((uint32_t)p_dir_entry_iter - (uint32_t)buff);
                break;
            }
            p_dir_entry_iter++;
        }
        pos += read_count;
    }

    int write_count = -1;
    if (delete_dir_entry_pos != -1) {
        // printf("debug delete_dir_entry write delete entry\n");
        // 标记一下视为删除
        p_dir_entry->f_type = FT_UNKNOWN;
        write_count = write_data_to_inode(p_part, p_dir->p_inode, p_all_block_lba, all_block_count, delete_dir_entry_pos, p_dir_entry, sizeof(struct Dir_entry));
    }

    sys_free(buff);
    sys_free(p_all_block_lba);
    return write_count > 0;
}

struct Dir_entry * dir_read(struct Dir *p_dir) {
    const bool enable_debug = false;
    if (!p_dir) {
        if (enable_debug) {
            printf("debug %s p_dir is NULL return NULL\n", __FILE__);
        }
        return NULL;
    }

    // 这种情况是读取完后的
    if (p_dir->p_dir_entry == NULL && p_dir->dir_pos != 0) {
        if (enable_debug) {
            printf("debug %s p_dir is read complete, dir_pos:%d return NULL\n", __FILE__, p_dir->dir_pos);
        }
        return NULL;
    }
    // 先从缓存内读取
    if (p_dir->p_dir_entry != NULL) {
        p_dir->p_dir_entry++;
        while((uint32_t)p_dir->p_dir_entry < (uint32_t)p_dir->dir_buff + DIR_CACHE_SIZE) {
            bool is_vaild_entry = p_dir->p_dir_entry->f_type != FT_UNKNOWN && p_dir->p_dir_entry->magic == DIR_ENTRY_MAGIC;
            if (enable_debug && is_vaild_entry) {
                printf("debug %s p_dir is read cache, item fileName:%s i_no:%d type:%d magic:%d vaild:%d\n", 
                    __FILE__, 
                    p_dir->p_dir_entry->fileName, 
                    p_dir->p_dir_entry->i_no, 
                    p_dir->p_dir_entry->f_type, 
                    p_dir->p_dir_entry->magic,
                    is_vaild_entry);
            }
            
            if (is_vaild_entry) {
                return p_dir->p_dir_entry;
            }
            p_dir->p_dir_entry++;
        }
    }
    // 这种情况下，说明缓存里的都是无效的, 从磁盘里读
    const struct Partition* p_part = g_current_part;
    assert(sizeof(struct Dir_entry) <= BLOCK_SIZE);
    Byte *buff = p_dir->dir_buff;
    uint32_t all_block_count;
    uint32_t *p_all_block_lba;
    get_dir_all_block_lba(p_part, p_dir, &p_all_block_lba, &all_block_count);
 
    const int32_t entry_count_in_block = BLOCK_SIZE / sizeof(struct Dir_entry);
    const int32_t max_used_read_size_in_once = entry_count_in_block * sizeof(struct Dir_entry);
     
    while(p_dir->dir_pos < p_dir->p_inode->i_size) {
        int32_t read_count = read_data_from_inode(p_part, p_dir->p_inode, p_all_block_lba, all_block_count, p_dir->dir_pos, buff, max_used_read_size_in_once);
        if(enable_debug) {
            printf("debug %s p_dir is read disk,i_size:%d dir_pos:%d max_used_read_size_in_once:%d read_count:%d\n", 
                __FILE__,p_dir->p_inode->i_size, p_dir->dir_pos, max_used_read_size_in_once, read_count);
        }

        if (read_count == -1 || read_count == 0) {
            break;
        }
        p_dir->dir_pos += read_count;
        // 开始遍历目录项
        struct Dir_entry *p_dir_entry_iter = (struct Dir_entry *)buff;
        while((uint32_t)p_dir_entry_iter < ((uint32_t)buff + read_count)) {
            if (enable_debug) {
                printf("debug %s p_dir is read disk, item fileName:%s i_no:%d type:%d magic:%d\n", 
                    __FILE__, 
                    p_dir_entry_iter->fileName, 
                    p_dir_entry_iter->i_no, 
                    p_dir_entry_iter->f_type, 
                    p_dir_entry_iter->magic
                );
            }
           if (p_dir_entry_iter->f_type != FT_UNKNOWN && p_dir_entry_iter->magic == DIR_ENTRY_MAGIC) {
                // 找到了就直接返回
                p_dir->p_dir_entry = p_dir_entry_iter;
                sys_free(p_all_block_lba);
                return p_dir->p_dir_entry;
            }
            p_dir_entry_iter++;
        }
    }
    // 这种情况下遍历完了没有找到的
    p_dir->p_dir_entry = NULL;
    sys_free(p_all_block_lba);
    return p_dir->p_dir_entry;
}

void dir_rewind(struct Dir *p_dir) {
    if (!p_dir) {
        return;
    }
    p_dir->p_dir_entry = NULL;
    p_dir->dir_pos = 0;
}

bool dir_is_empty(struct Dir *p_dir) {
    assert(p_dir);
    struct Dir *p_dir_iter = malloc(sizeof(struct Dir));
    int32_t count = 0;
    memcpy(p_dir_iter, p_dir, sizeof(struct Dir));
    dir_rewind(p_dir_iter);
    struct Dir_entry *p_dir_entry = NULL;
    while(p_dir_entry = dir_read(p_dir_iter)) {
        count++;
        // printf("debug dir_is_empty readdir count:%d p_dir_entry:0x%x name:%s type:%d  \n", count, p_dir_entry, p_dir_entry->fileName, p_dir_entry->f_type);
    }
    free(p_dir_iter);
    assert(count >= 2);
    return count == 2;
}

bool dir_delete(struct Dir *p_dir, struct Dir_entry *p_dir_entry, void* buff) {
    assert(p_dir && p_dir_entry);
    const bool enable_debug = false;
    if (enable_debug) {
        printf("%s start p_dir i_no:%d, delete:%s %d\n",__FILE__, p_dir->p_inode->i_no,p_dir_entry->fileName, p_dir_entry->i_no);
    }

    if (p_dir_entry->f_type != FT_DIRECTORY) {
        printf("dir_delete ftype is not FT_DIRECTORY, faild\n");
        return false;
    }
    struct Dir *p_delete_dir = dir_open(g_current_part, p_dir_entry->i_no);
    if (!dir_is_empty(p_delete_dir)) {
        dir_close(p_delete_dir);
        printf("dir_delete dir:%s is not empty, faild\n", p_dir_entry->fileName);
        return false;
    }
    dir_close(p_delete_dir);
    if (enable_debug) {
        printf("%s will inode_release dir_entry:%s %d\n", __FILE__, p_dir_entry->fileName, p_dir_entry->i_no);
    }
    bool res = inode_release(g_current_part, p_dir_entry->i_no);
    if (!res) {
        printf("dir_delete inode_release faild\n");
        return false;
    }
    if (enable_debug) {
        printf("%s will delete_dir_entry dir_entry:%s %d\n",__FILE__, p_dir_entry->fileName, p_dir_entry->i_no);
    }
    // 理论上不应该会失败，失败了也没什么办法回滚，就这样吧，直接断言一定成功
    res = delete_dir_entry(g_current_part, p_dir, p_dir_entry, buff);
    assert(res);
    if (enable_debug) {
        printf("%s end success p_dir i_no:%d, delete:%s %d\n",__FILE__, p_dir->p_inode->i_no,p_dir_entry->fileName, p_dir_entry->i_no);
    }
    return true;
}