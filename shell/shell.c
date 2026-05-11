#include "shell.h"
#include "debug.h"
#include "string.h"
#include "print.h"
#include "syscall.h"
#include "file.h"

// shell 单次最大的键入字符
#define MAX_CMD_LENGTH 512
// 加上命令本身的最大参数个数
#define MAX_ARG_COUNT 16
// 当前所在的目录名字的最大长度
#define MAX_CWD_LENGTH 64
// 存储输入的命令
static char g_cmd_line[MAX_CMD_LENGTH] = {0};

// 当前所在的目录名
char g_current_work_dir_name_cache[MAX_CWD_LENGTH] = {0};

// 输出提示符 
void print_prompt(void) {
    printf("[user@localhost %s]$ ", g_current_work_dir_name_cache);;
}

// 从键盘缓冲区读取并输出当前已经键入的字符
static void shell_readline(char *buff, int32_t count) {
    assert(buff && count);
    memset(buff, 0, count);
    char *pos = buff;
    while(read(stdin_no, pos, 1) != -1 && (pos - buff) < count) {
        switch (*pos)
        {
        case '\r':
        case '\n':
            *pos = 0;
            put_char('\n');
            return;
            /* code */
            break;
        case '\b':
            if (pos > buff) {
                pos--;
                *pos = 0;
                put_char('\b');
            }
            break;
        default:
            put_char(*pos);
            pos++;
            break;
        }
    }
    printf("\nreadline max read %d char, now is more than threshold\n", MAX_CMD_LENGTH);
}

void my_shell() {
    g_current_work_dir_name_cache[0] = '/';
    put_char('\n');
    while(1) {
        print_prompt();
        memset(g_cmd_line, 0, MAX_CMD_LENGTH);
        shell_readline(g_cmd_line, MAX_CMD_LENGTH);
        if (g_cmd_line[0] == 0) {
            continue;
        }
    }
}