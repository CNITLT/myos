LOADER_BASE_ADDR equ 0x900 
LOADER_STACK_TOP equ LOADER_BASE_ADDR
;启动的时候单个扇区指代512字节
LOADER_START_SECTOR equ 0x2 ;loader扇区起始地址
LOADER_SIZE_SECTOR equ 4 ;加载器占的扇区大小

KERNEL_PAGE_DIR_ADDR equ 0x400000 ;实际上4KB对齐就行了
KERNEL_BIN_BASE_ADDR equ 0x100000 ;内核文件加载地址,还不是能运行的程序，节还没展开对齐等
KERNEL_START_SECTOR equ 0x9 ;内核扇区地址
KERNEL_SIZE_SECTOR equ 200 ;内核文件大小，实际没这么大, 写大一点防止以后修改
KERNEL_TEXT_START equ 0xc0002000  ;内核的代码地址起始地址, 代码里没用到，链接时候指定，这里记下
KERNEL_STACK_TOP_ADDR equ 0xFC000000 ;内核栈顶的虚拟地址
KERNEL_FULL_STACK_TOP_ADDR equ 0xFB800000 ;内核栈满时的虚拟地址
DEVICE_DATA_ADDR equ 0x500 ;在实模式下读取的硬件设备信息存储的起始地址
ARDS_COUNT_ADDR equ 0x500 ;E820功能读取的ARDS数据结构数量
ARDS_ARR_ADDR equ 0x504 ;实际的ARDS数据结构数组的起点

PT_NULL equ 0x0
%include "gdt.inc"
%include "page.inc"






