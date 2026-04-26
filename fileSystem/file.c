#include "file.h"
#include "thread.h"

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