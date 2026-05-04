#include "fs.h"
#include "inode.h"
#include "super_block.h"
#include "dir.h"
#include "print.h"
#include "memory.h"
#include "string.h"
#include "debug.h"
#include "file.h"
#include "thread.h"
// 默认情况下的操作分区
struct Partition *g_current_part; 

void clean_part(struct Partition *part) {
    Byte *buff = (Byte *)sys_malloc(BLOCK_SIZE);
    memset(buff, 0, BLOCK_SIZE);
    // 跳过启动分区
    for(int i = 1; i < part->size_sector; i++) {
        ide_write(part->p_disk, part->start_lba, buff, 1);
    }
    sys_free(buff);
}

void partition_format(struct Partition *part) {
    clean_part(part);
    // 目前一个block和sector是等价的
    uint32_t boot_sector_sectors = 1;
    uint32_t super_block_sectors = 1;
    uint32_t inode_bitmap_sectors = DIV_ROUND_UP(MAX_FILES_PER_PART, BITS_PER_SECTOR);
    uint32_t inode_table_sectors = DIV_ROUND_UP((sizeof(struct Inode) * MAX_FILES_PER_PART), SECTOR_SIZE_BYTE);

    // 已使用的
    uint32_t used_sectors = boot_sector_sectors + super_block_sectors + inode_bitmap_sectors + inode_table_sectors;
    // 剩余的
    uint32_t free_sectors = part->size_sector - used_sectors;

    // 处理空闲位图，采用比较简单的方式计算
    //  真要按实际公式计算的话，应该是除 BITS_PER_SECTOR + 1，但数量级上差异不大，直接算吧，还简单点
    // 这么算其实有一部分是没有被管理的，不过无所谓了，简单点就简单点把
    uint32_t block_bitmap_sectors = DIV_ROUND_UP(free_sectors, BITS_PER_SECTOR);
    uint32_t block_bitmap_bit_len = free_sectors - block_bitmap_sectors; // 从空闲块减去位图的数量，即长度，有一定的冗余
    // 这里计算的误差，即没有被当成位图，也没有被当成空闲块，是幽灵块了
    block_bitmap_sectors = DIV_ROUND_UP(block_bitmap_bit_len, BITS_PER_SECTOR); 

    // 超级块填写， 按如下结构的视图
/*
    | OBR(1) | superBlock(1) | blockBitMap(?) | inodeBitMap(?)  |  inodeTable(?) | rootDir(视为数据块) | free |
*/
    struct Super_block *p_super_block = sys_malloc(sizeof(struct Super_block));
   
    p_super_block->magic = SUPER_BLOCK_MAGIC_NUMBER;  // 魔数
    p_super_block->size_sector = part->size_sector;  // 总扇区数
    p_super_block->inode_count = MAX_FILES_PER_PART; // inode总数
    p_super_block->partition_lba_base = part->start_lba; // 起点
    p_super_block->block_bitmap_lba = p_super_block->partition_lba_base + 2;     // 块位图起点
    p_super_block->block_bitmap_size_sector = block_bitmap_sectors; // 块位图大小

    p_super_block->inode_bitmap_lba = p_super_block->block_bitmap_lba + p_super_block->block_bitmap_size_sector; // inode位图起点
    p_super_block->inode_bitmap_size_sector = inode_bitmap_sectors; // inode位图大小

    p_super_block->inode_table_lba = p_super_block->inode_bitmap_lba + p_super_block->inode_bitmap_size_sector; // inode表起点
    p_super_block->inode_table_size_sector = inode_table_sectors; // inode表大小

    p_super_block->data_area_lba_base = p_super_block->inode_table_lba + p_super_block->inode_table_size_sector; // 数据块起点
    p_super_block->root_inode_no = 0; 
    p_super_block->dir_entry_size = sizeof(struct Dir_entry);
// --- debug ----
    printf("super_block size_sector:%d\n", p_super_block->size_sector);
    printf("super_block inode_count:%d\n", p_super_block->inode_count);
    printf("super_block partition_lba_base:0x%x\n", p_super_block->partition_lba_base);
    printf("super_block block_bitmap_lba:0x%x\n", p_super_block->block_bitmap_lba);
    printf("super_block block_bitmap_size_sector:0x%x\n", p_super_block->block_bitmap_size_sector);

    printf("super_block inode_bitmap_lba:0x%x\n", p_super_block->inode_bitmap_lba);
    printf("super_block inode_bitmap_size_sector:0x%x\n", p_super_block->inode_bitmap_size_sector);

    printf("super_block inode_table_lba:0x%x\n", p_super_block->inode_table_lba);
    printf("super_block inode_table_size_sector:0x%x\n", p_super_block->inode_table_size_sector);

    printf("super_block data_area_lba_base:0x%x\n", p_super_block->data_area_lba_base);
// --- debug ----

    // 超级块写入
    struct Disk *part_disk = part->p_disk;
    ide_write(part_disk, part->start_lba + 1, p_super_block, 1);
    
    // 申请位图数据库，以最大的为准，之后位图初始化可以重复使用
    uint32_t buf_size = MAX(p_super_block->block_bitmap_size_sector, p_super_block->inode_bitmap_size_sector);
    buf_size = MAX(buf_size, p_super_block->inode_table_size_sector);
    buf_size *= SECTOR_SIZE_BYTE;

    Byte *buff = (Byte *)sys_malloc(buf_size);
    memset(buff, 0, buf_size);

    // blockBitMap写入
    struct bitmap block_bitmap;
    block_bitmap.bits = buff;
    uint32_t real_bit_len = p_super_block->block_bitmap_size_sector;
    block_bitmap.len_bit = DIV_ROUND_UP(real_bit_len, 8) * 8; // 多出一些间距, 免得触发断言
    bitmap_init(&block_bitmap);
    // 第0个块是根目录，先占位
    bitmap_set(&block_bitmap, 0, BIT_STATE_USE);
    // 最后一个字节的处理，bitmap里没有实际对应的块，置1， 笔记最小存储单位是字节，不存在的位置1
    for (uint32_t i = real_bit_len; i < block_bitmap.len_bit; i++) {
       bitmap_set(&block_bitmap, i, BIT_STATE_USE);
    }
    // 写入磁盘
    ide_write(part_disk, p_super_block->block_bitmap_lba, block_bitmap.bits, p_super_block->block_bitmap_size_sector);

    // inodeBitMap 写入
    memset(buff, 0, buf_size);
    struct bitmap inode_bitmap;
    inode_bitmap.bits = buff;
    real_bit_len = p_super_block->inode_bitmap_size_sector;
    inode_bitmap.len_bit = DIV_ROUND_UP(real_bit_len, 8) * 8; // 多出一些间距, 免得触发断言
    bitmap_init(&inode_bitmap);
    // 第0个块是根目录，先占位
    bitmap_set(&inode_bitmap, 0, BIT_STATE_USE);
    // 最后一个字节的处理，bitmap里没有实际对应的块，置1， 笔记最小存储单位是字节，不存在的位置1
    for (uint32_t i = real_bit_len; i < inode_bitmap.len_bit; i++) {
       bitmap_set(&inode_bitmap, i, BIT_STATE_USE);
    }
    // 写入磁盘
    ide_write(part_disk, p_super_block->inode_bitmap_lba, inode_bitmap.bits, p_super_block->inode_bitmap_size_sector);
    //  inodeTable 写入
    memset(buff, 0, buf_size); 
    struct Inode *root_Inode = (struct  Inode *)buff;
    root_Inode->i_no = 0;
    root_Inode->i_size = p_super_block->dir_entry_size * 2;// .和..
    root_Inode->i_sectors[0] = p_super_block->data_area_lba_base;
    ide_write(part_disk, p_super_block->inode_table_lba, buff, p_super_block->inode_table_size_sector); 

    // 根目录项目写入 .和..
    memset(buff, 0, buf_size);  
    struct Dir_entry *p_now_entry =  (struct Dir_entry *)buff;
    init_dir_entry(p_now_entry, ".", 0, FT_DIRECTORY);

    struct Dir_entry *p_parent_entry = ++p_now_entry;
    memcpy(p_parent_entry->fileName, "..", 2);
    // 根目录只能指向自己
    init_dir_entry(p_now_entry, "..", 0, FT_DIRECTORY);

    ide_write(part_disk, p_super_block->data_area_lba_base, buff, 1);
    sys_free(buff);
    sys_free(p_super_block);
}


void fileSystem_init() {
    uint32_t channel_no = 0;
    uint32_t dev_no = 0;
    uint32_t part_index = 0;

    // 为没文件系统的分区格式化文件系统，除了操作系统在的分区
    struct Super_block *p_super_block = (struct  Super_block *)sys_malloc(SECTOR_SIZE_BYTE);
    assert(p_super_block);
   
    struct list_node* iter = g_partition_list.head.next;
    struct list_node* res = NULL;
    while(iter != &(g_partition_list.tail)){
        // 这里遍历的都是可以写数据的分区，忽略了操作系统代码占的分区
        struct Partition *p_part = elem2entry(struct Partition, part_tag, iter);
        if (p_part->size_sector > 0) {
            ide_read(p_part->p_disk, p_part->start_lba + 1, p_super_block, 1);
            printf("part:%s magic:0x%x size:%d\n", p_part->name, p_super_block->magic, p_part->size_sector);
            // 目录项可能会自己改一下，如果改过了重新格式化
            if (p_super_block->magic != SUPER_BLOCK_MAGIC_NUMBER || p_super_block->dir_entry_size != sizeof(struct Dir_entry)) {
                printf("part:%s init fileSystem\n", p_part->name);
                partition_format(p_part);
            } else {
                printf("part:%s has fileSystem skip init\n", p_part->name); 
            }
            // sleep_ms(2000);
        }
        iter = iter->next;
    }
    
    sys_free(p_super_block);

    // 初始化文件列表
    for(int i = 0; i < MAX_FD_SIZE; i++) {
        g_file_table[i].p_fd_inode = NULL;
    }

}


void load_partition(char *part_name) {
    // printf("debug enter load_partition\n");
    struct list_node* iter = g_partition_list.head.next;
    struct list_node* res = NULL;
    while(iter != &(g_partition_list.tail)){
        struct Partition * p_part = elem2entry(struct Partition, part_tag, iter);
        //printf("debug load_partition iter name:%s target:%s cmp:%d\n", p_part->name, part_name, strcmp(part_name, p_part->name));
        if (!strcmp(part_name, p_part->name)) {
            // 相等
            // 没有super_block说明之前没加载过，这里加载一下
            if (!p_part->super_block) {
                // 超级块信息加载
                p_part->super_block = sys_malloc(sizeof(struct Super_block));
                ide_read(p_part->p_disk, p_part->start_lba + 1, p_part->super_block, 1);

                // 位图信息加载
                uint32_t block_bit_map_size = (p_part->super_block->block_bitmap_size_sector) * SECTOR_SIZE_BYTE;
                Byte *p_block_bit_map = sys_malloc(block_bit_map_size);
                ide_read(p_part->p_disk, p_part->super_block->block_bitmap_lba, p_block_bit_map, p_part->super_block->block_bitmap_size_sector);
                p_part->block_bitmap.bits = p_block_bit_map;
                p_part->block_bitmap.len_bit = block_bit_map_size * 8;

                // inode位图信息加载
                uint32_t inode_bit_map_size = (p_part->super_block->inode_bitmap_size_sector) * SECTOR_SIZE_BYTE;
                Byte *p_inode_bit_map = sys_malloc(inode_bit_map_size);
                ide_read(p_part->p_disk, p_part->super_block->inode_bitmap_lba, p_inode_bit_map, p_part->super_block->inode_bitmap_size_sector);
                p_part->inode_bitmap.bits = p_inode_bit_map;
                p_part->inode_bitmap.len_bit = inode_bit_map_size * 8;
                // 链表初始化，其实这里算是重复初始化的，之前有过一次,保险点多一次也无所谓
                list_init(&p_part->opened_inodes);
                g_current_part = p_part;
                break;
            } else {
                // 加载过的直接改变指针就行
                 g_current_part = p_part;
                break;
            }
        }
        iter = iter->next;
    }
    if (!g_current_part) {
        printf("load part:%s failed\n", part_name);
        assert(g_current_part);
    } else {
        printf("load part:%s start lba:%d size sector:%d\n", g_current_part->name,g_current_part->start_lba, g_current_part->size_sector);
        printf("block bit map bits:0x%x len_bit:%d\n", g_current_part->block_bitmap.bits,  g_current_part->block_bitmap.len_bit); 
        printf("inode bit map bits:0x%x len_bit:%d\n", g_current_part->inode_bitmap.bits,  g_current_part->inode_bitmap.len_bit);  
        struct Super_block *super_block = g_current_part->super_block;
        printf("super block size_sector:%d inode_count:%d partition_lba_base:0x%x\n", super_block->size_sector, super_block->inode_count, super_block->partition_lba_base);  
        printf("super block block_bit_map_lba:0x%x size_sector:%d\n", super_block->block_bitmap_lba, super_block->block_bitmap_size_sector); 
        printf("super block inode_bit_map_lba:0x%x size_sector:%d\n", super_block->inode_bitmap_lba, super_block->inode_bitmap_size_sector); 
        printf("super block inode_table_lba:0x%x size_sector:%d\n", super_block->inode_table_lba, super_block->inode_table_size_sector); 
        printf("super block data_area_lba_base:0x%x root_inode_no:%d\n", super_block->data_area_lba_base,  super_block->root_inode_no);

        //sleep_ms(5000);
    }
}

void load_default_partition() {
    struct list_node* iter = g_partition_list.head.next;
    struct Partition *p_part = elem2entry(struct Partition, part_tag, iter); 
    load_partition(p_part->name);
}

char *path_parse(char *path, char *top_name_buff) {
    if (path[0] == '/') {
        while(*(++path) == '/');
    }

    while(*path != '/' && *path != 0) {
        *top_name_buff++ = *path++;
    }

    if (path[0] == 0) {
        return NULL;
    }
    return path;
}

int32_t path_depth(const char *path) {
    if (path == NULL) {
        return 0;
    }
    char *p = (char *)path;
    char name[MAX_FILE_NAME_LENGTH];
    memset(name, 0, MAX_FILE_NAME_LENGTH);
    int32_t depth = 0;
    p = path_parse(p, name);
    while(name[0]) {
        depth++;
        memset(name, 0, MAX_FILE_NAME_LENGTH);
        if (p) {
            p = path_parse(p, name);
        }
    }
    return depth;
}

int search_file(const char *path, struct Path_search_record *p_searched_record) {
    assert(p_searched_record != NULL);
    memset(p_searched_record, 0, sizeof(struct Path_search_record));
    if (!strcmp(path, "/") || !strcmp(path, "/.") || !strcmp(path, "/..")) {
        printf("search_file search target:%s is root dir cmp:%d %d %d\n",path, strcmp(path, "/"), strcmp(path, "/."), strcmp(path, "/.."));
        // 根目录及不存在的根目录的父目录的情况
        p_searched_record->p_parent_dir = &g_root_dir;
        p_searched_record->file_type = FT_DIRECTORY;
        p_searched_record->searched_path[0] = 0;
        return g_root_dir.p_inode->i_no;
    }

    int32_t path_len = strlen(path);
    assert(path_len < MAX_PATH_LENGTH && path_len > 1 && path[0] == '/');
    char *sub_path = (char *)path;
    struct Dir *p_parent_dir = &g_root_dir;
    struct Dir_entry dir_entry;

    char name[MAX_FILE_NAME_LENGTH] = {0};

    p_searched_record->p_parent_dir = p_parent_dir;
    p_searched_record->file_type = FT_UNKNOWN;
    int32_t parent_inode_no = p_parent_dir->p_inode->i_no;

    sub_path = path_parse(sub_path, name);
    while(name[0]) {
        //printf("debug search file sub_path:%s name:%s p_parent_dir:0x%x p_parent_dir->p_inode:0x%x  g_root_dir.p_inode:0x%x\n", sub_path, name, p_parent_dir, p_parent_dir->p_inode, g_root_dir.p_inode);
        assert(strlen(p_searched_record->searched_path) < MAX_PATH_LENGTH);
        strcat(p_searched_record->searched_path, "/");
        strcat(p_searched_record->searched_path, name); 
        if (search_dir_entry(g_current_part, p_parent_dir, name, &dir_entry)) {
            memset(name, 0, MAX_FILE_NAME_LENGTH);
            if (sub_path) {
                sub_path = path_parse(sub_path, name);
            }

            if (dir_entry.f_type == FT_DIRECTORY) {
                // 是目录继续查
                parent_inode_no = p_parent_dir->p_inode->i_no;
                dir_close(p_parent_dir);
                p_parent_dir = dir_open(g_current_part, dir_entry.i_no);
                p_searched_record->p_parent_dir = p_parent_dir;
                continue;
            } else if (dir_entry.f_type == FT_REGULLAR) {
                // 文件的话就返回,都是文件了，如果中间路径是文件，后面没法找，如果这里是最后路径，那么不需要找，无论如果是文件的话就可以返回了
                p_searched_record->file_type = FT_REGULLAR;
                return dir_entry.i_no;
            }
        } else {
            // 没有找到
            return -1;
        }
    }
    
    // 执行到这里说明是经过了完整的路径, 且只能是目录类型,文件会在中途返回
    // 此时这里是最后一个打开的也就是自己，需要关掉
    dir_close(p_searched_record->p_parent_dir);
    // 这里打开记录的父目录，重新修正下指向
    p_searched_record->p_parent_dir = dir_open(g_current_part, parent_inode_no);
    p_searched_record->file_type = FT_DIRECTORY;
    return dir_entry.i_no;
}

int32_t file_create(struct Dir *p_dir, char *filename, uint8_t flag) {
    void *buff = sys_malloc(2 * BLOCK_SIZE);
    if (!buff) {
        printf("file_create sys_malloc buff fail");
        return -1;
    }

    // 用于指定回滚步骤
    int32_t rollbakc_step = 0; 
    // 先分配一个inode号
    // printf("debug g_current_part inode bitmap bitlen:%d\n", g_current_part->inode_bitmap.len_bit);
    int32_t inode_no = inode_bitmap_alloc(g_current_part);
    if (inode_no == -1) {
        printf("file_create inode_bitmap_alloc fail");
        return -1;
    }

    int fd_index = -1;
    struct Inode *p_inode = NULL;
    do {
        p_inode = (struct Inode *)sys_malloc_in_kernel(sizeof(struct Inode));
        if (!p_inode) {
            printf("file_create sys_malloc inode fail");
            rollbakc_step = 1;
            break;
        }
        inode_init(p_inode, inode_no);
        
        // 获取有无空闲的操槽位
        fd_index = get_free_file_slot_in_g_table();
        if (fd_index == -1) {
            printf("file_create get_free_file_slot_in_g_table fail");
            rollbakc_step = 2;
            break;
        }

        g_file_table[fd_index].p_fd_inode = p_inode;
        g_file_table[fd_index].fd_pos = 0;
        g_file_table[fd_index].fd_flag = flag;
        g_file_table[fd_index].p_fd_inode->write_deny = false;

        struct Dir_entry dir_entry;
        memset(&dir_entry, 0 ,sizeof(struct Dir_entry));
        init_dir_entry(&dir_entry, filename, inode_no, FT_REGULLAR);
        // printf("debug file_create sync_dir_entry\n ");
        if (!sync_dir_entry(g_current_part, p_dir, &dir_entry, buff)) {
            printf("file_create sync_dir_entry fail");
            rollbakc_step = 3;
            break;
        }

        // 这里与书上不一样，sync_dir_entry改过了，写入成功会自动写入inode, 所以此处没有写目录inode的步骤
        // 写入文件的inode
        memset(buff, 0, 2*BLOCK_SIZE);
        inode_sync(g_current_part, p_inode, buff);
        // 同步bitmap
        bitmap_sync(g_current_part, inode_no, Bitmap_type_inode);


        // 最后在进程内打开失败的话，则认为文件是创建成功的，但打开失败了
        int pcb_fd_index = pcb_fd_install(fd_index);
        if (pcb_fd_index != -1) {
            // 成功，追加到打开的inode链表
            list_push_front(&g_current_part->opened_inodes, &p_inode->inode_tag);
            p_inode->i_open_cnts = 1;
        } else {
            // 失败释放对应资源，但对于磁盘则不回滚
            g_file_table[fd_index].p_fd_inode = NULL;
            sys_free_in_kernel(p_inode);
        }

        sys_free(buff);
        return pcb_fd_index;
    } while(0);



    // 这里基本不用break，回滚步骤就是如此
    switch (rollbakc_step)
    {
    case 3:
        memset(g_file_table + fd_index, 0 , sizeof(struct File));
    case 2:
        sys_free_in_kernel(p_inode);
    case 1:
        // inode空间分配失败，回滚inode_bit_map
        inode_bitmap_free(g_current_part, inode_no);
    default:
        break;
    }

    sys_free(buff);
    return -1;
}


int32_t sys_open(const char *path, uint8_t flags) {
    assert(path);
    assert(flags < 8);
    // 末尾是/ 说明是目录，这个函数不支持，直接返回
    if (path[strlen(path) - 1] == '/') {
        printf("sys_open path is dir, open faild");
        return -1;
    }
    int32_t fd = -1;
    // 偏大的变量尽量别放栈里，容易爆栈
    struct Path_search_record *p_path_search_record = sys_malloc(sizeof(struct Path_search_record));
    if (!p_path_search_record) {
        printf("sys_open malloc Path_search_record faild\n");
        return -1;
    }
    memset(p_path_search_record, 0 , sizeof(struct Path_search_record));
    int32_t origin_path_depth = path_depth(path);
    // printf("debug sys_open will entry search file\n");
    int inode_no = search_file(path, p_path_search_record);
    bool found = inode_no != -1;
    if (p_path_search_record->file_type == FT_DIRECTORY) {
        // 最终结果是目录或者在中途查找断链，中间结果是目录, 此类情况直接结束
        printf("sys_open p_path_search_record is dir, faild\n");
        dir_close(p_path_search_record->p_parent_dir);
        sys_free(p_path_search_record);
        return -1;
    }
    //printf("debug sys_open search inode_no:%d search_path:%s file_type:%d \n", inode_no, p_path_search_record->searched_path, p_path_search_record->file_type);

    int32_t searched_path_depth = path_depth(p_path_search_record->searched_path);
    if (searched_path_depth != origin_path_depth) {
        // 深度不一样，说明中间是断了
        printf("sys_open %s is not exist\n", path);
        dir_close(p_path_search_record->p_parent_dir);
        sys_free(p_path_search_record);
        return -1; 
    }

    // 深度一样，在最后一个情况下可能出现的问题
    // 没有找到, 且没有指定创建, 返回-1
    if (!found && !(flags & O_CREAT)) {
        printf("sys_open %s is not exist and not create flag\n", path);
        dir_close(p_path_search_record->p_parent_dir);
        sys_free(p_path_search_record);
        return -1; 
    }  
    // 找到了，但是要创建，也认为是失败
    if (found && (flags & O_CREAT)) {
        printf("sys_open %s is exist but set create flag\n", path);
        dir_close(p_path_search_record->p_parent_dir);
        sys_free(p_path_search_record);
        return -1;  
    }

    // printf("debug sys_open enter file_create\n");
    // printf("debug sys_open p_dir->inode:%x\n", p_path_search_record->p_parent_dir->p_inode);
	switch (flags & O_CREAT) {
        case O_CREAT:
            fd = file_create(p_path_search_record->p_parent_dir, strrchr(path, '/') + 1, flags);
            break;
        default:
            fd = file_open(inode_no, flags);
            break;
    }

    dir_close(p_path_search_record->p_parent_dir);
    sys_free(p_path_search_record);
    return fd;
}


int32_t fd_local2global(uint32_t local_fd_index) {
    assert(local_fd_index >= USED_FD_START_INDEX);
    struct task_struct *pcb = get_current_pcb();
    int32_t global_fd_index = pcb->fd_table[local_fd_index];
    return global_fd_index;
}

int32_t sys_close(int32_t local_fd_index) {
    int ret = -1;
    if (local_fd_index >= USED_FD_START_INDEX) {
        int32_t global_fd_index = fd_local2global(local_fd_index);
        ret = file_close(&g_file_table[global_fd_index]);
        struct task_struct *pcb = get_current_pcb();
        pcb->fd_table[local_fd_index] = -1;
    }
    return ret;
}

int32_t sys_write(int32_t fd, const void *data, size_t count) {
    if (fd < 0) {
        printf("sys_write fd error, less than 0\n");
        return -1;
    }
    
    if (fd == stdout_no) {
        char *str = sys_malloc(count + 1);
        str[count] = 0;
        memcpy(str, data, count);
        put_str(str);
        return count;
    }

    uint32_t global_fd_index = fd_local2global(fd);
    struct File* p_file = &g_file_table[global_fd_index];
    //printf("debug sys_write p_file inode_no:%d fd:%d g_fd:%d\n", p_file->p_fd_inode->i_no, fd, global_fd_index);

    if (p_file->fd_flag & O_WR_ONLY || p_file->fd_flag & O_RDWR) {
        // 具有写权限
        uint32_t write_count = file_write(p_file, data, count);
        return write_count;
    } else {
        printf("sys_write not allow to write\n");
        return -1;
    }
}

int32_t sys_read(int32_t fd, void *data, size_t count) {
    if (fd < 0) {
        printf("sys_read read faild for fd less than 0 \n");
        return -1;
    }
    uint32_t global_fd_index = fd_local2global(fd);
    struct File* p_file = &g_file_table[global_fd_index];
    return file_read(p_file, data, count);
}

int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence) {
    if (fd < 0) {
        printf("sys_lseek fd error less than 0\n");
        return -1;
    }
    assert(whence >= 1 && whence <=3);
    uint32_t global_fd = fd_local2global(fd);
    struct File* p_file = g_file_table + global_fd;
    int32_t new_pos = 0;
    int32_t file_size = (int32_t) p_file->p_fd_inode->i_size;
    switch (whence)
    {
    case SEEK_SET:
        new_pos = offset;
        break;
    case SEEK_CUR:
        new_pos = (int32_t)p_file->fd_pos + offset;
    case SEEK_END:
        new_pos = file_size + offset;
    default:
        break;
    }
    new_pos = MAX(0, new_pos);
    new_pos = MIN(file_size, new_pos);
    p_file->fd_pos = new_pos;
    return p_file->fd_pos;
}

int32_t sys_unlink(const char *path) {
    assert(strlen(path) < MAX_PATH_LENGTH);
    // 先检查文件是否存在
    struct Path_search_record *p_path_search_record = sys_malloc(sizeof(struct Path_search_record));
    memset(p_path_search_record, 0, sizeof(struct Path_search_record));
    int inode_no = search_file(path, p_path_search_record);
    bool is_found = (path_depth(path) == path_depth(p_path_search_record->searched_path)) && inode_no != -1;
    // 根目录不能删除
    assert(inode_no != 0);
    int32_t res = -1;
    do {
        if (!is_found) {
            printf("sys_unlink file %s is not found \n", path);
            break;
        }

        if (p_path_search_record->file_type != FT_REGULLAR) {
            printf("sys_unlink file %s is not file \n", path);
            break;
        }

        char *filename = strrchr(path, '/') + 1;
        struct Dir_entry dir_entry = {0};
        bool search_res = search_dir_entry(g_current_part, p_path_search_record->p_parent_dir, filename, &dir_entry);
        if (!search_res) {
            printf("sys_unlink can't found file in dir\n");
            break;
        }
        bool delete_res = file_delete(p_path_search_record->p_parent_dir, &dir_entry, NULL);
        if(!delete_res) {
            printf("sys_unlink file_delete faild\n");
            break;
        }
        res = 0;
    }while (0);

    dir_close(p_path_search_record->p_parent_dir);
    sys_free(p_path_search_record);
    return res;
}

int32_t sys_mkdir(const char *path) {
    assert(strlen(path) < MAX_PATH_LENGTH);
    assert(2 * sizeof(struct Dir_entry) < BLOCK_SIZE);
    // 先检查路径是否存在
    struct Path_search_record *p_path_search_record = sys_malloc(sizeof(struct Path_search_record));
    memset(p_path_search_record, 0, sizeof(struct Path_search_record));
    int inode_no = search_file(path, p_path_search_record);
    bool has_parent_dir = (path_depth(path) == path_depth(p_path_search_record->searched_path));
    bool is_found = has_parent_dir && inode_no != -1;
    // 不能创建根目录
    assert(inode_no != 0);
    int32_t res = -1;
    int32_t buff_size = 2 * BLOCK_SIZE;
    Byte *buff = (Byte *)sys_malloc(buff_size);
    do {
        if (!buff) {
            printf("sys_mkdir malloc buff faild\n", path);
            break;
        }
        if (!has_parent_dir) {
            printf("sys_mkdir dir %s can't recursion make\n", path);
            break;
        }

        if (is_found) {
            printf("sys_mkdir dir %s is exit \n", path);
            break;
        }

        if (p_path_search_record->file_type == FT_REGULLAR) {
            printf("sys_mkdir dir %s is file \n", path);
            break;
        }

        // 说明前面的检查通过, 开始创建流程
        // 用于指定回滚步骤
        int32_t rollbakc_step = 0;
        // 先分配一个inode号
        int32_t inode_no = inode_bitmap_alloc(g_current_part);
        if (inode_no == -1) {
            printf("sys_mkdir inode_bitmap_alloc fail\n");
            break;
        }

        struct Inode *p_inode = NULL;
        struct Dir *p_parent_dir = p_path_search_record->p_parent_dir;
        uint32_t *p_all_block_lba = NULL;
        uint32_t all_block_lba_count = 0;
        do {
            // 这里的顺序和书上不一样
            // 先分配inode
            p_inode = (struct Inode *)sys_malloc_in_kernel(sizeof(struct Inode));
            if (!p_inode) {
                printf("file_create sys_malloc inode fail");
                rollbakc_step = 1;
                break;
            }
            inode_init(p_inode, inode_no);

            memset(buff, 0, buff_size);
            inode_sync(g_current_part, p_inode, buff);
            // 同步bitmap
            bitmap_sync(g_current_part, inode_no, Bitmap_type_inode);
            
            // 向父目录写入目录项
            memset(buff, 0, buff_size);
            struct Dir_entry *p_dir_entry = (struct Dir_entry*)buff;
            init_dir_entry(p_dir_entry, strrchr(path, '/') + 1, inode_no, FT_DIRECTORY);
            if (!sync_dir_entry(g_current_part, p_parent_dir, p_dir_entry, buff + BLOCK_SIZE)) {
                printf("sys_mkdir sync_dir_entry parent dir_entry faild\n");
                rollbakc_step = 2;
                break;
            }

            // 向目录代表的inode内写入初始数据
            memset(buff, 0, buff_size);
            init_dir_entry(p_dir_entry, ".", inode_no, FT_DIRECTORY);
            p_dir_entry++;
            init_dir_entry(p_dir_entry, "..", p_parent_dir->p_inode->i_no, FT_DIRECTORY);

            
            get_inode_all_block_lba(g_current_part, p_inode, &p_all_block_lba, &all_block_lba_count);
            // 然后写入目录内容
            int write_count = write_data_to_inode(g_current_part, p_inode, p_all_block_lba, all_block_lba_count, 0, buff, 2 * sizeof(struct Dir_entry));
            assert(write_count > 0);

            sys_free_in_kernel(p_inode);
            sys_free(p_all_block_lba);
            res = 0;
        } while(0);


        // 这里基本不用break，回滚步骤就是如此
        switch (rollbakc_step) {
            case 2:
                inode_release(g_current_part, inode_no);
                sys_free_in_kernel(p_inode);
            case 1:
                // inode空间分配失败，回滚inode_bit_map
                inode_bitmap_free(g_current_part, inode_no);
                res = -1;
            default:
                break;
        }
    } while (0);

    dir_close(p_path_search_record->p_parent_dir);
    sys_free(p_path_search_record);
    sys_free(buff);
    return res;
}

struct Dir *sys_opendir(const char *path) {
    assert(strlen(path) < MAX_PATH_LENGTH);
    if (!strcmp(path, "/") || !strcmp(path, "/.") || !strcmp(path, "/..")) {
        // 能判断是根目录的情况
        // 再打开一份，不直接返回g_root_dir
        return dir_open(g_current_part, g_current_part->super_block->root_inode_no);
    }
    struct Path_search_record *p_path_search_record = sys_malloc(sizeof(struct Path_search_record));
    memset(p_path_search_record, 0, sizeof(struct Path_search_record));
    int inode_no = search_file(path, p_path_search_record);
    bool has_parent_dir = (path_depth(path) == path_depth(p_path_search_record->searched_path));
    bool is_found = has_parent_dir && inode_no != -1;

    struct Dir *p_dir_res = NULL;
    do {
        if (!is_found) {
            printf("sys_opendir path %s not found\n", path);
            break;
        }
        if (p_path_search_record->file_type != FT_DIRECTORY) {
            printf("sys_opendir path %s is not dir\n", path);
            break;
        }
        p_dir_res = dir_open(g_current_part, inode_no);
    }while(0);

    dir_close(p_path_search_record->p_parent_dir);
    sys_free(p_path_search_record);
    return p_dir_res;
}

int32_t sys_closedir(struct Dir *p_dir) {
    if (!p_dir) {
        return 0;
    }
    dir_close(p_dir);
    return 0;
}