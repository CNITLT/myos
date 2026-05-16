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

// 如果本身是绝对路径，则返回清洗后的
// 如果是相对路径，则先拼接当前工作目录，然后返回清洗后的
// 返回值需要外部释放
static char *get_target_absolute_path(char *argv_path) {
    char *buff = malloc(MAX_PATH_LENGTH);
    assert(buff);
    memset(buff,0 , MAX_PATH_LENGTH);
    // 先获取当前路径
    if (argv_path[0] == '/') {
        // 如果是/开头的绝对路径那么清洗后直接切换
        wash_path(argv_path, buff);
    } else {
        // 其他情况，先获取当前路径，然后和给的参数拼一起
        getcwd(buff, MAX_PATH_LENGTH);
        strcat(buff, "/");
        strcat(buff, argv_path);
        char *buff2 = malloc(MAX_PATH_LENGTH);
        assert(buff2);
        memset(buff2, 0, MAX_PATH_LENGTH);
        wash_path(buff, buff2);
        memset(buff,0 , MAX_PATH_LENGTH);
        memcpy(buff, buff2, strlen(buff2));
        free(buff2);
    }     
    return buff;
}
/*
 * @brief shell内建命令，打印当前目录
*/
void buildin_pwd(int argc, char *argv[]) {
    if (argc != 1) {
        printf("pwd not support argument\n");
        return;
    }
    char *buff = malloc(MAX_PATH_LENGTH);
    assert(buff);
    memset(buff,0 , MAX_PATH_LENGTH);
    getcwd(buff, MAX_PATH_LENGTH);
    printf("%s\n", buff);
    free(buff);
}

/*
 * @brief shell内建命令，切换目录
*/
void buildin_cd(int argc, char *argv[]) {
    if (argc > 2) {
        return;
    }
    if (argc == 1) {
        chdir("/");
    } else {
        char *buff = get_target_absolute_path(argv[1]);
        chdir(buff);
        free(buff);
    }
}

/*
 * @brief shell内建命令，列出当前目录项, 就只支持-l
*/
void buildin_ls(int argc, char *argv[]) {
    if (argc > 2) {
        printf("ls not support more than 2 param\n");
        return;
    }

    if (argc == 1) {
        // 一个的话就循环打印出当前的目录项吧
        char *cwd = getcwd(NULL, 0);
        struct Dir *p_dir = opendir(cwd);
        struct Dir_entry *p_dir_entry = NULL;
        char *buff = malloc(17);
        assert(buff);
        while(p_dir_entry = dir_read(p_dir)) {
            memset(buff,' ' ,17);
            memcpy(buff, p_dir_entry->fileName, strlen(p_dir_entry->fileName));
            printf("%s", buff);
            buff[16] = 0;
        }
        free(buff);
        printf("\n");
        free(cwd);
    } else if (argc == 2 && !strcmp("-l", argv[1])) {
        char *cwd = getcwd(NULL, 0);
        struct Dir *p_dir = opendir(cwd);
        struct Dir_entry *p_dir_entry = NULL;

        printf("type    inode   size      name\n");
        char *buff = malloc(17);
        assert(buff);
        char *entry_path = malloc(MAX_PATH_LENGTH);
        while(p_dir_entry = dir_read(p_dir)) {
            // 类型 dir 和 file
            memset(buff,0 ,17);
            memset(buff,' ',8);
            if (p_dir_entry->f_type == FT_REGULLAR) {
                printf("FILE    ");
            } else {
                printf("DIR     ");
            }

            memset(buff,0 , 17);
            memset(buff,' ',8);
            sprintf(buff, "%d", p_dir_entry->i_no);
            printf("%s", buff);

            memset(buff,0 , 17);
            memset(buff,' ',10);
            
            memset(entry_path, 0, MAX_PATH_LENGTH);
            memcpy(entry_path, cwd, strlen(cwd));
            strcat(entry_path, "/");
            strcat(entry_path, p_dir_entry->fileName);
            char *abs_path = get_target_absolute_path(entry_path);
            struct Stat entry_stat = {0};
            stat(abs_path, &entry_stat);
            free(abs_path);
         
            sprintf(buff, "%dKB", entry_stat.st_size);
            printf("%s", buff);

            memset(buff,0 , 17);
            memset(buff,' ',16);
            sprintf(buff, "%s", p_dir_entry->fileName);
            printf("%s", buff);
            
            printf("\n");
        }
       
        printf("\n");
        free(cwd);
        free(entry_path);
        free(buff);
    } else {
        printf("ls not support %s\n", argv[1]);
    }
}

void buildin_ps(int argc, char *argv[]) {
    if (argc > 1) {
        printf("ps not support %s\n", argv[1]);
        return;
    }
    ps();
}