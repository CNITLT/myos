#ifndef __SHELL_BUILD_CMD_H
#define __SHELL_BUILD_CMD_H


/*
 * @brief 清洗路径，例:将/a/b/./../c 清洗为/a/c
 * @param old_path: char *: 旧的绝对路径
 * @param new_path: char *: 新绝对路径
*/
void wash_path(char *old_path, char *new_path);

#endif