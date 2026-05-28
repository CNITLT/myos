#ifndef __SYS_CALL_H
#define __SYS_CALL_H
#include "stdint.h"
#include "stddef.h"
#include "syscall_init.h"
struct Dir;
struct Dir_entry;
struct Stat;
enum SYSCALL_NR {
    SYS_GETPID = 0,
    SYS_MALLOC,
    SYS_FREE,
    SYS_WRITE,
    SYS_READ,
    SYS_LSEEK,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_UNLINK,
    SYS_OPENDIR,
    SYS_CLOSEDIR,
    SYS_READDIR,
    SYS_REWINDDIR,
    SYS_RMDIR,
    SYS_GETCWD,
    SYS_CHDIR,
    SYS_STAT,
    SYS_FORK,
    SYS_PS,
    SYS_EXECV,
    SYS_MKDIR,
    SYS_THREAD_YIELD,
    SYS_WAIT,
    SYS_EXIT
 };
/*
@brief 用户态的系统调用入口
@param syscallNum: syscall_param_type : 本质就是int 系统调用号
@param ... 可变参数，看系统调用号的不同而不同，目前只支持32位大小，最多3个额外参数
@return syscall_ret_type 本质int32_t 返回值，具体函数看系统调用的不同而不同
*/
syscall_ret_type syscall(syscall_param_type syscallNum, ...);

//这里是系统调用的代理，用户态可用

/*
@brief 返回当前线程的PID
@return pid_t 线程PID
*/
pid_t getpid(void);


/*
@brief 分配指定大小的内存空间
@param size: size_t :字节为单位的空间大小
@return void *:可用空间的首地址
*/
void* malloc(size_t size);

/*
@brief 释放malloc分配的空间
@param p: void * :将要释放的sys_malloc分配的地址
*/
void free(void* p);


/*
 * @brief 写入数据到特定的文件描述符
 * @param fd: int32_t :待写入的文件描述符索引
 * @param data: void *: 待写入的数据 
 * @param count: size_t :写入的数据量
 * @return 成功返回写入的数据量，失败返回-1
*/
int32_t write(int32_t fd, const void *data, size_t count);


/*
 * @brief 读取文件描述符的数据
 * @param fd: int32_t :待读取的文件描述符索引
 * @param data: void *: 待读取的数据的缓冲区
 * @param count: size_t :读取的数据量
 * @return 成功返回读取的数据量，失败返回-1
*/
int32_t read(int32_t fd, const void *data, size_t count);

/*
 * @brief 调整文件描述符里的游标
 * @param fd: int32_t :待读取的文件描述符索引
 * @param offset: int32_t : 偏移量
 * @param whence: uint8_t : 枚举值，偏移的起点
 * @return int32_t 新的游标位置, 失败返回-1
*/
int32_t lseek(int32_t fd, int32_t offset, uint8_t whence);

/*
 * @brief 打开或创建文件
 * @param path: const char *: 文件绝对路径
 * @param flags: uint8_t: 对应的操作权限
 * @return int32_t 进程级文件描述符，失败返回-1
*/
int32_t open(const char *path, uint8_t flags);

/*
 * @brief 关闭文件描述符
 * @param fd: int32_t : 待关闭的文件描述符索引
 * @return 成功返回0， 否则-1
*/
int32_t close(int32_t fd);

/*
 * @brief 删除非目录类型的文件
 * @param path: const char *: 文件路径
 * @return 成功返回0， 否则-1
*/
int32_t unlink(const char *path);


/*
 * @brief 打开一个目录
 * @param path: const char *: 目录路径
 * @return 成功返回指向dir结构的指针，否则NULL
*/
struct Dir *opendir(const char *path);

/*
 * @brief 关闭一个目录
 * @param p_dir: struct Dir*: 目录指针
 * @return 成功0，否则-1
*/
int32_t closedir(struct Dir *p_dir);

/*
 * @brief 读取目录项，读取后移动到下一个,直到返回NULL
 * @param param p_dir: struct Dir *: 目录信息
 * @return struct Dir_entry *目录项, 读取结束后返回NULL
*/
struct Dir_entry * readdir(struct Dir *p_dir);

/*
 * @brief 重置目录的读取游标
 * @param param p_dir: struct Dir *: 目录信息
*/
void rewinddir(struct Dir *p_dir);

/*
    @brief 删除空目录，对有内容的目录无法删除
    @param path: const char *: 目录路径
    @return 成功0， 失败-1
*/
int32_t rmdir(const char *path);

/*
 * @brief 创建目录
 * @param path: const char *: 目录路径
 * @return 成功0， 失败-1
*/
int32_t mkdir(const char *path);

/*
 * @brief 获取当前工作路径
 * @param buff: char *: 若不为null，则会将当前路径写入该buff内
 * @param size: size_t :缓存区大小
 * @return char *, 若buff不为null，则返回buff, 若buff为null，则内部动态分配，外部释放
*/
char * getcwd(char *buff, size_t size);

/*
 * @brief 切换当前工作路径
 * @param path: char *path: 绝对路径
 * @return 成功0，否则-1
*/
int32_t chdir(const char *path);

/*
 * @brief 获取绝对路径下的文件信息
 * @param path: const char *: 文件或目录的绝对路径
 * @param p_stat: struct Stat *: 文件结构信息，非空
 * @return 成功0，否则-1
*/
int32_t stat(const char *path, struct Stat *p_stat);


/*
 * @brief 复制一个子进程，父进程返回子进程pid, 子进程返回0， 失败返回-1
*/
pid_t fork();

/*
 * @brief 执行一个新的程序，替换当前进程的内存映像
 * @param path: 程序路径
 * @param argv: 参数数组
 * @return 成功返回0，失败返回-1
*/
int32_t execv(const char* path, char* const argv[]);


/*
 * @brief 打印所有进程信息
 */
void ps();

/*
 * @brief 主动让出CPU
 */
void thread_yield();

/*
 * @brief 等待子进程退出，返回子进程的PID
 * @param exit_status: int32_t *: 存放子进程退出状态的指针，可为NULL
 * @return pid_t 成功返回子进程PID，失败返回-1
*/
pid_t wait(int32_t *exit_status);

/*
 * @brief 退出当前进程
 * @param exit_status: int32_t : 退出状态码
*/
void exit(int32_t exit_status);
#endif