#include "build_cmd.h"
#include "debug.h"
#include "dir.h"
#include "memory.h"
#include "syscall.h"
#include "string.h"

void wash_path(char *old_path, char *new_path) {
    assert(old_path[0] == '/');
    new_path[0] = '/';
    new_path[1] = 0;
    char *name = malloc(MAX_FILE_NAME_LENGTH + 1);
    char *path = old_path;

    // 下面处理过程中，整体保证除了根目录外， 最后一位不是/
    while((memset(name, 0, MAX_FILE_NAME_LENGTH + 1), path = path_parse(path, name), name[0] != 0)) {
        if (name[0] == 0) {
            // 说明到头了
            return;
        } else if (!strcmp(name, ".")) {
            continue;
        } else if (!strcmp(name, "..")) {
            if (!strcmp(new_path, "/")) {
                continue;
            } else {
                // 从后面向前面删除一个目录层级
                char *iter = strrchr(new_path, '/');
                // 保留根目录的/不动
                if (iter == new_path) {
                    iter++;
                }
                while(*iter) {
                    *iter = 0;
                    iter++;
                }
                
            }
        } else {
            // 不是根目录的话才额外加/
            if (strcmp(new_path, "/")) {
                strcat(new_path, "/");
            }
            strcat(new_path, name);
        }
    }
    free(name);
}