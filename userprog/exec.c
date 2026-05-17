#include "exec.h"
#include "file.h"
#include "fs.h"
#include "page.h"
#include "debug.h"

int sys_exec(const char* path, char* const argv[]) {
    // 空实现，暂时返回-1表示未实现
    return -1;
}

bool segment_load(int32_t fd, uint32_t offset, uint32_t filesz, uint32_t vaddr) {
    const bool enable_debug = false;
    uint32_t vaddr_start_page = vaddr & 0xFFFFF000;
    uint32_t start_in_page = vaddr - vaddr_start_page;
    // 先计算下需要多少页，然后开始分配
    uint32_t vaddr_end_page = (vaddr + filesz) & 0xFFFFF000;
    uint32_t page_count = (vaddr_end_page - vaddr_start_page) / PAGE_SIZE + 1;
    // 然后开始分配页
    struct task_struct *pcb = get_current_pcb();
    uint32_t page_attr = PAGE_P_ATTR_EXIST | PAGE_RW_ATTR_RW | PAGE_US_ATTR_USER;
    // 先检查页是否存在
    for (int i = 0; i < page_count; i++) {
        // 对不存在的页开始分配
        uint32_t user_page_vaddr = vaddr_start_page + i * PAGE_SIZE;
        page *p_page_dir_entry = get_page_dir_entry_vaddr(user_page_vaddr, PAGE_DIR_VADDR);
        
        // 没有对应的页表，映射一个页表， 存在的话把也权限全开了，不搞什么只读的，怎方便怎么来
        if(p_page_dir_entry->P == PAGE_P_VALUE_UNEXIST){
            vaddr_t res = malloc_page_core(user_page_vaddr, 1, &pcb->vmemory_pool, PAGE_DIR_VADDR, page_attr);
            if (res == NULL) {
                printf("%s malloc_page_core vaddr:0x%x false \n", __FILE__, user_page_vaddr);
                return false;
            }
            continue;
        } else {
             uint32_t* p_uint32_page_dir_entry = (uint32_t*)p_page_dir_entry;
            *p_uint32_page_dir_entry &= 0xFFFFF000;
            *p_uint32_page_dir_entry |= (page_attr&0xFFF);
        }        

        page *p_page_table_entry = get_page_table_entry_vaddr(user_page_vaddr, PAGE_DIR_VADDR);
        if (p_page_table_entry->P == PAGE_P_VALUE_UNEXIST) {
            vaddr_t res = malloc_page_core(user_page_vaddr, 1, &pcb->vmemory_pool, PAGE_DIR_VADDR, page_attr);
             if (res == NULL) {
                printf("%s malloc_page_core vaddr:0x%x false \n", __FILE__, user_page_vaddr);
                return false;
            }
            assert(res);
        } else {
            uint32_t* p_uint32_page_table_entry = (uint32_t*)p_page_table_entry;
            *p_uint32_page_table_entry &= 0xFFFFF000;
            *p_uint32_page_table_entry |= (page_attr&0xFFF);
        }
    }
    // 重新激活刷新下硬件缓存
    page_dir_activate(pcb);

    // 然后读取文件内容开始复制
    sys_lseek(fd, offset, SEEK_SET);
    Byte * buff = sys_malloc_in_kernel(BLOCK_SIZE);
    int32_t read_count = 0;

    while (read_count < filesz) {
        int read_in_once = sys_read(fd, buff, MIN(BLOCK_SIZE, filesz - read_count));
        if (read_in_once <= 0) {
            sys_free_in_kernel(buff);
            printf("%s read_in_once false \n",__FILE__, read_in_once);
            return false;
        }
        memcpy((void*)(vaddr + read_count), buff, read_in_once);
        read_count += read_in_once;
    }
    
    sys_free_in_kernel(BLOCK_SIZE);
}