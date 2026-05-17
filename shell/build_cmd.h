#ifndef __SHELL_BUILD_CMD_H
#define __SHELL_BUILD_CMD_H


/*
 * @brief 清洗路径，例:将/a/b/./../c 清洗为/a/c
 * @param old_path: char *: 旧的绝对路径
 * @param new_path: char *: 新绝对路径
*/
void wash_path(char *old_path, char *new_path);

// 如果本身是绝对路径，则返回清洗后的
// 如果是相对路径，则先拼接当前工作目录，然后返回清洗后的
// 返回值需要外部释放
char *get_target_absolute_path(char *argv_path);

/*
 * @brief shell内建命令，打印当前目录
*/
void buildin_pwd(int argc, char *argv[]);

/*
 * @brief shell内建命令，切换目录
*/
void buildin_cd(int argc, char *argv[]);

/*
 * @brief shell内建命令，列出当前目录项
*/
void buildin_ls(int argc, char *argv[]);

/*
 * @brief shell内建命令，打印当前所有的进程信息
*/
void buildin_ps(int argc, char *argv[]);


/*
 * @brief shell内建命令，清空当前屏幕
*/
void buildin_clear(int argc, char *argv[]);


/*
 * @brief shell内建命令，创建目录
*/
void buildin_mkdir(int argc, char *argv[]);



/*
 * @brief shell内建命令，删除目录
*/
void buildin_rmdir(int argc, char *argv[]);



/*
 * @brief shell内建命令，删除文件
*/
void buildin_rm(int argc, char *argv[]);

#endif