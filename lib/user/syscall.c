#include "syscall.h"


syscall_ret_type syscall(syscall_param_type syscallNum, ...){
    syscall_ret_type ret = 0;
    syscall_param_type* args = &syscallNum;
    //这三个参数值可能是随机的，看当时的栈是什么，有用的话实际的函数会取的到，没用的话也不会用到，用三个是为了不区分调用形式
    //这里最多就浪费点指令和空间
    syscall_param_type arg1 = args[1];
    syscall_param_type arg2 = args[2];
    syscall_param_type arg3 = args[3];
    asm volatile(" \
        int $0x80 \
        ":"=a"(ret) \
        :"a"(syscallNum), "b"(arg1), "c"(arg2),"d"(arg3));
    return ret;
}

pid_t getpid(void){
    return syscall(SYS_GETPID);
}

void* malloc(size_t size){
    return syscall(SYS_MALLOC, size);
}

void free(void* p){
    syscall(SYS_FREE, p);
}

int32_t write(int32_t fd, const void *data, size_t count) {
    return syscall(SYS_WRITE, fd, data, count);
}

int32_t read(int32_t fd, const void *data, size_t count) {
    return syscall(SYS_READ, fd, data, count);
}

int32_t lseek(int32_t fd, int32_t offset, uint8_t whence) {
    return syscall(SYS_LSEEK, fd, offset, whence);
}

int32_t unlink(const char *path) {
    return syscall(SYS_UNLINK, path);
}

struct Dir *opendir(const char *path) {
    return syscall(SYS_OPENDIR, path);
}

int32_t closedir(struct Dir *p_dir) {
    return syscall(SYS_CLOSEDIR, p_dir);
}