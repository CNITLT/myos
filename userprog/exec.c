#include "exec.h"
#include "file.h"
#include "fs.h"
#include "page.h"
#include "debug.h"
#include "elf32.h"
#include "process.h"
#include "shell.h"
#include "build_cmd.h"
#include "string.h"
int sys_execv(const char* path, char* const argv[]) {
    const bool enable_debug = true;
    int32_t argc = 0;
    while(argv[argc]) {
        argc++;
    }
    uint32_t entry_point = 0;
    if (enable_debug) {
        printf("%s will call get_target_absolute_path:%s\n", __FILE__, path);
    }
    char *abs_path = get_target_absolute_path(path);
    if (enable_debug) {
        printf("%s will call sys_load:%s\n", __FILE__, abs_path);
    }
    struct task_struct* new_pcb = sys_load(abs_path, &entry_point);
    if (new_pcb == NULL) {
        if (enable_debug) {
            printf("%s sys_load false\n", __FILE__);
        }
        free(abs_path);
        return -1;
    }
    // 加载成功，切换到新的进程
    struct task_struct* pcb = get_current_pcb();
    char *name = strrchr(abs_path, '/');
    
    memcpy(pcb->name, name, strlen(name));
    pcb->name[TASK_NAME_LENGTH - 1] = 0;
    // TODO:构建中断栈, 这里的逻辑应该要改下, 之前就魔改过，再对着书上来可能有问题
    struct interrupt_stack *p_intr_stack = (struct interrupt_stack *)((uint32_t)pcb + PAGE_SIZE - sizeof(struct interrupt_stack));
    p_intr_stack->ebx = (uint32_t)argv;
    p_intr_stack->ecx = argc;
    p_intr_stack->eip = entry_point;
    p_intr_stack->esp = (void *)(USER_STACK3_VADDR + PAGE_SIZE - 16);
    if (enable_debug) {
        printf("%s will free abs_path:0x%x\n", __FILE__, abs_path);
    }
    free(abs_path);
    if (enable_debug) {
        printf("%s will free_kernel_page new_pcb:0x%x\n", __FILE__, new_pcb);
    }
    free_kernel_page(new_pcb, 1);
    // block_desc重置
    mem_block_desc_array_init(&pcb->u_block_desc);
    // 页表切换
    page_dir_activate(new_pcb);
    // 直接从中断返回
    intr_exit_from(p_intr_stack);
    return 0;
}

bool segment_load(struct task_struct *new_pcb, int32_t fd, uint32_t offset, uint32_t filesz, uint32_t vaddr) {
    const bool enable_debug = true;
    uint32_t vaddr_start_page = vaddr & 0xFFFFF000;
    uint32_t start_in_page = vaddr - vaddr_start_page;
    // 先计算下需要多少页，然后开始分配
    uint32_t vaddr_end_page = (vaddr + filesz) & 0xFFFFF000;
    uint32_t page_count = (vaddr_end_page - vaddr_start_page) / PAGE_SIZE + 1;
    // 然后开始分配页
    uint32_t page_attr = PAGE_P_ATTR_EXIST | PAGE_RW_ATTR_RW | PAGE_US_ATTR_USER;
    // 先检查页是否存在
    struct task_struct *pcb = get_current_pcb();

    // 切换页表
    page_dir_activate(new_pcb);
    for (int i = 0; i < page_count; i++) {
        // 对不存在的页开始分配
        uint32_t user_page_vaddr = vaddr_start_page + i * PAGE_SIZE;
        page *p_page_dir_entry = get_page_dir_entry_vaddr(user_page_vaddr, PAGE_DIR_VADDR);
        if (enable_debug) {
            printf("%s i:%d/%d user_page_vaddr:0x%x\n", __FILE__, i, page_count, user_page_vaddr);
        }
        // 没有对应的页表，映射一个页表， 存在的话把也权限全开了，不搞什么只读的，怎方便怎么来
        if(p_page_dir_entry->P == PAGE_P_VALUE_UNEXIST){
            vaddr_t res = malloc_page_core(user_page_vaddr, 1, &new_pcb->vmemory_pool, PAGE_DIR_VADDR, page_attr);
            if (res == NULL) {
                printf("%s malloc_page_core vaddr:0x%x false \n", __FILE__, user_page_vaddr);
                return false;
            }
            // 分配页成功，地址必须匹配，不然视为失败，可能原因是一开始用户进程非空
            assert(res == user_page_vaddr);
            if (enable_debug) {
                printf("%s malloc_page_core success vaddr:0x%x\n", __FILE__, user_page_vaddr);
            }
            continue;
        } else {
             uint32_t* p_uint32_page_dir_entry = (uint32_t*)p_page_dir_entry;
            *p_uint32_page_dir_entry &= 0xFFFFF000;
            *p_uint32_page_dir_entry |= (page_attr&0xFFF);
        }        

        page *p_page_table_entry = get_page_table_entry_vaddr(user_page_vaddr, PAGE_DIR_VADDR);
        if (p_page_table_entry->P == PAGE_P_VALUE_UNEXIST) {
            vaddr_t res = malloc_page_core(user_page_vaddr, 1, &new_pcb->vmemory_pool, PAGE_DIR_VADDR, page_attr);
            if (res == NULL) {
                printf("%s malloc_page_core vaddr:0x%x false \n", __FILE__, user_page_vaddr);
                return false;
            }
            if (enable_debug) {
                printf("%s malloc_page_core success vaddr:0x%x\n", __FILE__, user_page_vaddr);
            }
            assert(res == user_page_vaddr);
        } else {
            uint32_t* p_uint32_page_table_entry = (uint32_t*)p_page_table_entry;
            *p_uint32_page_table_entry &= 0xFFFFF000;
            *p_uint32_page_table_entry |= (page_attr&0xFFF);
        }
    }
 
    // 之后还要读，但读的时候要保证使用正确的页目录，因为内部一些分配用的是sys_malloc，而不是内核的，需要保证激活的CR3和PCB是匹配的
    page_dir_activate(pcb);
    // 然后读取文件内容开始复制（文件系统内部缓冲区已改为 sys_malloc_in_kernel，不依赖页目录上下文）
    sys_lseek(fd, offset, SEEK_SET);
    Byte * buff = sys_malloc_in_kernel(BLOCK_SIZE);
    assert(buff);
    int32_t read_count = 0;

    while (read_count < filesz) {
        if (enable_debug) {
            printf("%s will call sys_read:%d 0x%x %d read_count:%d\n", __FILE__, fd, buff, MIN(BLOCK_SIZE, filesz - read_count), read_count);
        }
      
        int read_in_once = sys_read(fd, buff, MIN(BLOCK_SIZE, filesz - read_count));
        if (enable_debug) {
            printf("%s did call sys_read res:%d\n", __FILE__, read_in_once);
        }
        if (read_in_once <= 0) {
            sys_free_in_kernel(buff);
            printf("%s read_in_once false, ret:%d\n", __FILE__, read_in_once);
            return false;
        }
        // 写的时候换成新的PCB
        page_dir_activate(new_pcb);
        memcpy((void*)(vaddr + read_count), buff, read_in_once);
        page_dir_activate(pcb);
        read_count += read_in_once;
    }

    sys_free_in_kernel(buff);
    return true;
}


struct task_struct *sys_load(const char * path, uint32_t *p_entry_point) {
    const bool enable_debug = true;
    int fd = sys_open(path, O_RD_ONLY);
    if (fd == -1) {
        printf("%s Failed to open file: %s\n",__FILE__, path);
        return NULL;
    }
    // 先检测是否是elf32的可执行文件
    struct Elf32_Ehdr elf_header = {0};
    memset(&elf_header, 0, sizeof(struct Elf32_Ehdr));
    sys_lseek(fd, 0, SEEK_SET);
    int read_count = sys_read(fd, &elf_header, sizeof(struct Elf32_Ehdr));
    if (read_count != sizeof(struct Elf32_Ehdr)) {
        sys_close(fd);
        printf("%s read_count != sizeof(struct Elf32_Ehdr)\n", __FILE__);
        return NULL;
    }

    // 检查下ELF头是否正确
     if (memcmp(elf_header.e_ident, "\177ELF\1\1\1", 7) \
      || elf_header.e_type != 2 \
      || elf_header.e_machine != 3 \
      || elf_header.e_version != 1 \
      || elf_header.e_phnum > 1024 \
      || elf_header.e_phentsize != sizeof(struct Elf32_Phdr)) {
        sys_close(fd);
        printf("%s is not right ELFHeader\n", __FILE__); 
        printf("e_ident:%s\n", elf_header.e_ident);

        printf("e_type:0x%x\n", elf_header.e_type);

        printf("e_machine:0x%x\n", elf_header.e_machine);
        printf("e_version:0x%x\n", elf_header.e_version);

        printf("e_phnum:0x%x\n", elf_header.e_phnum);

        printf("e_phentsize:0x%x\n", elf_header.e_phentsize);
        return NULL;
   }

   // 准备一个新的pcb用做加载
    struct task_struct* new_pcb = malloc_kernel_page(1);
    //初始化PCB信息
    if (enable_debug) {
        printf("%s will call init_pcb new_pcb:0x%x\n",__FILE__, new_pcb);
    }
    init_pcb(new_pcb, "load", USER_PROCESS_DEFAULT_PRIOR); 
    if (enable_debug) {
        printf("%s will call user_vmemory_pool_init:0x%x\n",__FILE__, &new_pcb->vmemory_pool);
    }
    user_vmemory_pool_init(&new_pcb->vmemory_pool);
    if (enable_debug) {
        printf("%s will call mem_block_desc_array_init:0x%x\n",__FILE__, &new_pcb->u_block_desc);
    }
    mem_block_desc_array_init(&new_pcb->u_block_desc);
    //页表创建
    if (enable_debug) {
        printf("%s will call create_page_dir\n",__FILE__);
    }
    new_pcb->page_dir = create_page_dir();
    if (enable_debug) {
        printf("%s will call thread_create e_entry:0x%x\n",__FILE__, elf_header.e_entry);
    }
    thread_create(new_pcb, start_process, elf_header.e_entry);
    // 遍历所有程序段, 将可加载的加载到内存中
    struct Elf32_Phdr prog_header = {0};
    Elf32_Off prog_header_offset = elf_header.e_phoff;
    Elf32_Half prog_header_size = elf_header.e_phentsize;
    Elf32_Half prog_header_count = elf_header.e_phnum;
    
    for (int i = 0; i < prog_header_count; i++) {
            memset(&prog_header, 0 , sizeof(struct Elf32_Phdr));
            sys_lseek(fd, prog_header_offset + i * prog_header_size, SEEK_SET);
            int read_count = sys_read(fd, &prog_header, sizeof(struct Elf32_Phdr));
            if (read_count != sizeof(struct Elf32_Phdr)) {
                sys_close(fd);
                free_kernel_page(new_pcb, 1);
                printf("%s read_count != sizeof(struct Elf32_Phdr)\n", __FILE__);
                return NULL;
            }
            
            if (prog_header.p_type == PT_LOAD) {
                if (!segment_load(new_pcb, fd, prog_header.p_offset, prog_header.p_filesz, prog_header.p_vaddr)) {
                    sys_close(fd);
                    free_kernel_page(new_pcb, 1);
                    printf("%s segment_load p_offset:0x%x p_filesz:%d p_vaddr:0x%x\n", __FILE__, prog_header.p_offset, prog_header.p_filesz, prog_header.p_vaddr);
                    return NULL;
                }
            }
    }

    sys_close(fd);
    if (p_entry_point) {
        *p_entry_point = elf_header.e_entry;
    }
    return new_pcb;
}