#include "shell.h"
#include "debug.h"
#include "string.h"
#include "print.h"
#include "syscall.h"
#include "file.h"
#include "keyboard.h"
#include "build_cmd.h"

// shell 单次最大的键入字符
#define MAX_CMD_LENGTH 512
// 加上命令本身的最大参数个数
#define MAX_ARG_COUNT 16
// 当前所在的目录名字的最大长度
#define MAX_CWD_LENGTH 64

// 存储输入的命令
static char g_cmd_line[MAX_CMD_LENGTH] = {0};
static char *g_argv[MAX_ARG_COUNT] = {0};
static size_t g_argc = 0;
// 当前所在的目录名
char g_current_work_dir_name_cache[MAX_CWD_LENGTH] = {0};

// 输出提示符
void print_prompt(void)
{
    printf("[user@localhost %s]$ ", g_current_work_dir_name_cache);
    ;
}

// 从键盘缓冲区读取并输出当前已经键入的字符
static void shell_readline(char *buff, int32_t count)
{
    assert(buff && count);
    memset(buff, 0, count);
    char *pos = buff;
    while (read(stdin_no, pos, 1) != -1 && (pos - buff) < count)
    {
        switch (*pos)
        {
        case '\r':
        case '\n':
        case KEY_CTRL('c'):
            *pos = 0;
            put_char('\n');
            return;
            /* code */
            break;
        case '\b':
            if (pos > buff)
            {
                pos--;
                *pos = 0;
                put_char('\b');
            }
            break;
        case KEY_CTRL('u'):
            while (pos != buff)
            {
                *pos = 0;
                pos--;
                put_char('\b');
            }
            *pos = 0;
            break;
        case KEY_CTRL('l'):
            *pos = 0;
            clear_screen();
            print_prompt();
            printf("%s", buff);
            break;
        default:
            put_char(*pos);
            pos++;
            break;
        }
    }
    printf("\nreadline max read %d char, now is more than threshold\n", MAX_CMD_LENGTH);
}

// 对g_cmd_line内的字符串以空格进行分割，并将结果存入g_argv_line内，会对g_cmd_line有所修改，空格的位置会被写入0
// 返回分割的个数
static size_t cmd_parse()
{
    char separator = ' ';
    char *str = g_cmd_line;
    char **arr = g_argv;
    g_argc = 0;
    memset(arr, NULL, MAX_ARG_COUNT);
    char *next = str;
    char **arr_next = arr;
    size_t count = 0;
    while (*next == separator)
    {
        next++;
    }

    while (*next && count < MAX_ARG_COUNT)
    {
        while (*next == separator)
        {
            *next = 0;
            next++;
        }

        // 先找到下一个首字母开头
        if (*next && *next != separator)
        {
            *arr_next = next;
            arr_next++;
            count++;
            while (*next && *next != separator)
            {
                next++;
            }
        }
    }
    g_argc = count;
    return count;
}

static void debug_parse()
{
    for (int i = 0; i < g_argc; i++)
    {
        printf("parse %d/%d arg:%s\n", i, g_argc, g_argv[i]);
    }
}

static void flush_cwd_name_cache()
{
    memset(g_current_work_dir_name_cache, 0, MAX_CWD_LENGTH);
    g_current_work_dir_name_cache[0] = '/';
    char *cwd = getcwd(NULL, 0);
    char *name = strrchr(cwd, '/');
    if (strcmp("/", cwd))
    {
        name++;
        // printf("%s name:%s cwd:%s\n", __FILE__, name, cwd);
        memset(g_current_work_dir_name_cache, 0, MAX_CWD_LENGTH);
        memcpy(g_current_work_dir_name_cache, name, strlen(name));
    }
    free(cwd);
}

static void execute_cmd(void)
{
    if (!strcmp("ps", g_argv[0]))
    {
        buildin_ps(g_argc, g_argv);
    }
    else if (g_argc == 2 && !strcmp("wash_path", g_argv[0]))
    {
        char *new_path = malloc(MAX_CMD_LENGTH);
        memset(new_path, 0, MAX_CMD_LENGTH);
        wash_path(g_argv[1], new_path);
        printf("wash_path %s to %s\n", g_argv[1], new_path);
        free(new_path);
    }
    else if (!strcmp("ls", g_argv[0]))
    {
        buildin_ls(g_argc, g_argv);
    }
    else if (!strcmp("pwd", g_argv[0]))
    {
        buildin_pwd(g_argc, g_argv);
    }
    else if (!strcmp("cd", g_argv[0]))
    {
        buildin_cd(g_argc, g_argv);
        flush_cwd_name_cache();
    }
    else if (!strcmp("clear", g_argv[0]))
    {
        buildin_clear(g_argc, g_argv);
    }
    else if (!strcmp("mkdir", g_argv[0]))
    {
        buildin_mkdir(g_argc, g_argv);
    }
    else if (!strcmp("rmdir", g_argv[0]))
    {
        buildin_rmdir(g_argc, g_argv);
    }
    else if (!strcmp("rm", g_argv[0]))
    {
        buildin_rm(g_argc, g_argv);
    }
    else
    {
        // 从磁盘上找
        printf("shell not found cmd:%s\n", g_argv[0]);
    }
}
void my_shell()
{
    put_char('\n');
    flush_cwd_name_cache();
    while (1)
    {
        print_prompt();
        memset(g_cmd_line, 0, MAX_CMD_LENGTH);
        shell_readline(g_cmd_line, MAX_CMD_LENGTH);
        if (g_cmd_line[0] == 0)
        {
            continue;
        }
        cmd_parse();
        // debug_parse();
        if (!g_argv[0])
        {
            continue;
        }

        execute_cmd();
    }
}