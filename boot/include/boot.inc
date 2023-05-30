LOADER_BASE_ADDR equ 0x900 
LOADER_STACK_TOP equ LOADER_BASE_ADDR
LOADER_START_SECTOR equ 0x2 ;loader扇区起始地址
LOADER_SIZE_SECTOR equ 4 ;加载器占的扇区大小

KERNEL_PAGE_DIR_ADDR equ 0x100000 ;要求4MB对齐
KERNEL_BIN_BASE_ADDR equ 0x70000 ;内核文件加载地址,还不是能运行的程序，节还没展开对齐等
KERNEL_START_SECTOR equ 0x9 ;内核扇区地址
KERNEL_SIZE_SECTOR equ 200 ;内核文件大小，实际没这么大, 写大一点防止以后修改
KERNEL_TEXT_START equ 0xc0002000  ;内核的代码地址起始地址, 代码里没用到，链接时候指定，这里记下
KERNEL_STACK_TOP_INIT equ 0xc009f000

PT_NULL equ 0x0
%include "gdt.inc"
%include "page.inc"






