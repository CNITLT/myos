#ifndef __USERPROG_ELF32_H
#define __USERPROG_ELF32_H

#include "stdint.h"

/* ELF32 文件结构定义 */
typedef uint32_t Elf32_Word;
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef uint32_t Elf32_Half;

/* ELF 魔数 */
#define ELF_MAG0         0x7F
#define ELF_MAG1         'E'
#define ELF_MAG2         'L'
#define ELF_MAG3         'F'
#define ELF_MAGIC        "\x7F""ELF"

/* ELF 文件类型 */
#define ET_NONE          0      /* 无文件类型 */
#define ET_REL           1      /* 可重定位文件 */
#define ET_EXEC          2      /* 可执行文件 */
#define ET_DYN           3      /* 共享目标文件 */
#define ET_CORE          4      /* 核心文件 */
#define ET_LOPROC        0xFF00 /* 处理器特定范围起始 */
#define ET_HIPROC        0xFFFF /* 处理器特定范围结束 */

/* ELF 机器类型 */
#define EM_NONE          0      /* 无机器 */
#define EM_386           3      /* Intel 80386 */
#define EM_486           6      /* Intel 80486 */
#define EM_X86_64        62     /* AMD x86-64 */

/* ELF 版本 */
#define EV_NONE          0      /* 无效版本 */
#define EV_CURRENT       1      /* 当前版本 */

/* ELF 程序头类型 */
#define PT_NULL          0      /* 未使用 */
#define PT_LOAD          1      /* 可加载段 */
#define PT_DYNAMIC       2      /* 动态链接信息 */
#define PT_INTERP        3      /* 解释器路径名 */
#define PT_NOTE          4      /* 辅助信息 */
#define PT_SHLIB         5      /* 保留 */
#define PT_PHDR          6      /* 程序头表自身 */
#define PT_LOPROC        0x70000000 /* 处理器特定范围起始 */
#define PT_HIPROC        0x7FFFFFFF /* 处理器特定范围结束 */

/* 程序头标志 */
#define PF_X             0x1    /* 可执行 */
#define PF_W             0x2    /* 可写 */
#define PF_R             0x4    /* 可读 */
#define PF_MASKPROC      0xF0000000 /* 处理器特定标志掩码 */

/* 节头类型 */
#define SHT_NULL         0      /* 未使用 */
#define SHT_PROGBITS     1      /* 程序数据 */
#define SHT_SYMTAB       2      /* 符号表 */
#define SHT_STRTAB       3      /* 字符串表 */
#define SHT_RELA         4      /* 带加数的重定位项 */
#define SHT_HASH         5      /* 符号哈希表 */
#define SHT_DYNAMIC      6      /* 动态链接信息 */
#define SHT_NOTE         7      /* 辅助信息 */
#define SHT_NOBITS       8      /* 不占用文件空间 */
#define SHT_REL          9      /* 无加数的重定位项 */
#define SHT_SHLIB        10     /* 保留 */
#define SHT_DYNSYM       11     /* 动态链接符号表 */
#define SHT_LOPROC       0x70000000 /* 处理器特定范围起始 */
#define SHT_HIPROC       0x7FFFFFFF /* 处理器特定范围结束 */
#define SHT_LOUSER       0x80000000 /* 应用程序特定范围起始 */
#define SHT_HIUSER       0xFFFFFFFF /* 应用程序特定范围结束 */

/* 节头标志 */
#define SHF_WRITE        0x1    /* 可写 */
#define SHF_ALLOC        0x2    /* 占用内存 */
#define SHF_EXECINSTR    0x4    /* 可执行 */
#define SHF_MASKPROC     0xF0000000 /* 处理器特定标志掩码 */

/* ELF32 文件头 */
typedef struct Elf32_Ehdr{
    unsigned char   e_ident[16];   /* ELF 标识 */
    Elf32_Half      e_type;        /* 文件类型 */
    Elf32_Half      e_machine;     /* 机器类型 */
    Elf32_Word      e_version;     /* 版本 */
    Elf32_Addr      e_entry;       /* 入口点虚拟地址 */
    Elf32_Off       e_phoff;       /* 程序头表偏移 */
    Elf32_Off       e_shoff;       /* 节头表偏移 */
    Elf32_Word      e_flags;       /* 处理器特定标志 */
    Elf32_Half      e_ehsize;      /* ELF 头大小 */
    Elf32_Half      e_phentsize;   /* 程序头表项大小 */
    Elf32_Half      e_phnum;       /* 程序头表项数量 */
    Elf32_Half      e_shentsize;   /* 节头表项大小 */
    Elf32_Half      e_shnum;       /* 节头表项数量 */
    Elf32_Half      e_shstrndx;    /* 节名字符串表索引 */
} Elf32_Ehdr;

/* ELF32 程序头 */
typedef struct Elf32_Phdr{
    Elf32_Word p_type;    /* 段类型 */
    Elf32_Off  p_offset;  /* 段在文件中的偏移 */
    Elf32_Addr p_vaddr;   /* 段虚拟地址 */
    Elf32_Addr p_paddr;   /* 段物理地址 */
    Elf32_Word p_filesz;  /* 段在文件中的大小 */
    Elf32_Word p_memsz;   /* 段在内存中的大小 */
    Elf32_Word p_flags;   /* 段标志 */
    Elf32_Word p_align;   /* 段对齐 */
} Elf32_Phdr;

/* ELF32 节头 */
typedef struct Elf32_Shdr{
    Elf32_Word sh_name;      /* 节名索引（在节名字符串表中） */
    Elf32_Word sh_type;      /* 节类型 */
    Elf32_Word sh_flags;     /* 节标志 */
    Elf32_Addr sh_addr;      /* 节虚拟地址 */
    Elf32_Off  sh_offset;    /* 节在文件中的偏移 */
    Elf32_Word sh_size;      /* 节大小 */
    Elf32_Word sh_link;      /* 链接到其他节 */
    Elf32_Word sh_info;      /* 附加信息 */
    Elf32_Word sh_addralign; /* 节对齐 */
    Elf32_Word sh_entsize;   /* 项大小（如符号表项大小） */
} Elf32_Shdr;

#endif /* __USERPROG_ELF32_H */