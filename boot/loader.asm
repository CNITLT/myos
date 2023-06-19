%include "include/boot.inc"

DISK_LBA28_LOW_PORT_ADDR equ 0x1F3
DISK_LBA28_MID_PORT_ADDR equ 0x1F4
DISK_LBA28_HIGH_PORT_ADDR equ 0x1F5
DISK_LBA28_EXHIGH_PORT_ADDR equ 0x1F6 ;最高的4位LBA地址，写入到这个寄存器的低4位
DISK_DEVICE_PORT_ADDR equ 0x1F6
DISK_COMMAND_PORT_ADDR equ 0x1F7
DISK_COMMAND_READ equ 0x20
DISK_COMMAND_WRITE equ 0x30
DISK_COMMAND_IDENTIFY equ 0xEC
DISK_DATA_PORT_ADDR equ 0x1F0

SECTION loader vstart=LOADER_BASE_ADDR
[bits 16] ;还没开启保护模式，目前还是16位
jmp loader_start

times 64 - ($ - $$) db 0;为了让GDT64位对齐
;定义下GDT
GDT_BASE:
    dd 0x00000000
    dd 0x00000000

GDT_CODE:
    dd 0x0000FFFF ;低32位
    dd (0x0<<24) | GDT_G_4K | GDT_D_CODE_32 | GDT_L_32 | (0xF << 16) | GDT_P_EXIST | GDT_DPL_0 | GDT_S_TYPE_CODE | (0x00);高32位 4K粒度， 32位代码， 存在， DPL为0， 非一致性代码段

GDT_DATA:
    dd 0x0000FFFF;低32位
    dd (0x0<<24) | GDT_G_4K | GDT_B_STACK_32 | GDT_L_32 | (0xF << 16) | GDT_P_EXIST | GDT_DPL_0 | GDT_S_TYPE_DATA | GDT_TYPE_DATA_W | GDT_TYPE_DATA_EXTEND_UP | (0x00);高32位 4K粒度， 32位栈， 存在， DPL为0， 可读可写，向上扩展数据段

GDT_SIZE   equ   $ - GDT_BASE
GDT_LIMIT   equ   GDT_SIZE - 1

times 60 dq 0					 ; 此处预留60个描述符的空位(slot)
GDT_SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL_0         ;代码段选择子
GDT_SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL_0	 ; 数据段选择子

;以下是定义gdt的指针，前2字节是gdt界限，后4字节是gdt起始地址
gdt_ptr  dw  GDT_LIMIT 
    dd  GDT_BASE


loader_start:
    ;获取内存地址
    ;-------  int 15h eax = 0000E820h ,edx = 534D4150h ('SMAP') 获取内存布局  -------
   mov dword [ARDS_COUNT_ADDR], 0
   xor ebx, ebx		      ;第一次调用时，ebx值要为0
   mov edx, 0x534d4150	      ;edx只赋值一次，循环体中不会改变
   mov di, ARDS_ARR_ADDR	      ;ards结构缓冲区
.e820_mem_get_loop:	      ;循环获取每个ARDS内存范围描述结构
   mov eax, 0x0000e820	      ;执行int 0x15后,eax值变为0x534d4150,所以每次执行int前都要更新为子功能号。
   mov ecx, 20		      ;ARDS地址范围描述符结构大小是20字节
   int 0x15
   add di, cx		      ;使di增加20字节指向缓冲区中新的ARDS结构位置
   inc word [ARDS_COUNT_ADDR]	      ;记录ARDS数量
   cmp ebx, 0		      ;若ebx为0且cf不为1,这说明ards全部返回，当前已是最后一个
   jnz .e820_mem_get_loop

    ;开启保护模式, 这里只开启了分段，分页还没开启
    ;开启A20地址线
    ;加载GDTR
    ; 将cr0的pe位置1

    ;1.开启A20地址线
    in al,0x92
    or al,0000_0010B
    out 0x92,al
    ;加载GDTR寄存器
    lgdt [gdt_ptr]
    
    ;cr0寄存器PE位置1
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax

    ;刷新流水线和缓存
    ;虽然开启了保护模式，但是段缓存寄存器里面的值还是16位下的缓存，这里还是按照16位模式执行的
    jmp dword GDT_SELECTOR_CODE:pe_mode_start

[bits 32]
pe_mode_start:
    ;刷新其它的选择子
    mov ax, GDT_SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov ss, ax
    mov esp,LOADER_STACK_TOP
    mov ebp,LOADER_STACK_TOP
    ;创建页目录和页表
    call setup_kernel_page_dir_and_table
    ;准备开启分页机制
    ;根据GDTR的地址找GDT的时候，如果开启分页的话，会要经过页表的转化
    ;将之前1MB之下的GDT地址改到0XC000 0000 之上,加上0xC000 0000，其实不改也行
    ;这段不用了，重新排布了内存布局
    ;sgdt [gdt_ptr]
    ;mov eax, [gdt_ptr + 2]
    ;add eax, 0XC000_0000
    ;mov [gdt_ptr + 2], eax
    ;栈指针也加一下
    ;add esp, 0xC000_0000
    ;add ebp, 0xC000_0000

    ;将页目录地址赋值给CR3， 4K对齐的，只有高20位表示地址， 存的是真实的物理地址
    ;---------31~12-----------11~5--------4-------3-------2~0---
    ;|  物理地址(31~12)    |  未定义  |   PCD  |  PWT  | 未定义 |
    ;-----------------------------------------------------------
    mov eax, KERNEL_PAGE_DIR_ADDR
    and eax, 0xFFFF_F000
    mov cr3, eax

    ;CR0的PG位打开,开启分页机制
    mov eax, cr0
    or eax, 0x8000_0000
    mov cr0, eax

    ;重新加载下gdtr寄存器
    ;重新排布内存后，不需要了
    ;lgdt [gdt_ptr]

    mov eax, KERNEL_START_SECTOR
    mov ebx, KERNEL_BIN_BASE_ADDR
    mov ecx, KERNEL_SIZE_SECTOR
    call read_disk_32
    
    ;再初始化内核
    call kernel_init

    ;跳到内核运行
    mov esp, KERNEL_STACK_TOP_ADDR - 32 ;调整下栈的位置, 32字节是个缓冲区，用于将ESP和EBP隔开
    mov ebp, KERNEL_STACK_TOP_ADDR
    ;读取入口地址
    mov eax, [KERNEL_BIN_BASE_ADDR+24]
    jmp eax
    

    


loop_end:
   jmp loop_end




setup_kernel_page_dir_and_table:
    pushad
    ;创建页目录和页表
    ;虚拟地址3GB往上16mb全映射到低端8-24MB，虚拟地址低端4MB映射到物理地址低端4MB
    ;物理地址4-8MB,映射到虚拟地址最后4MB,是页目录和页表
    ;先清空页目录和页表所在的整个4MB空间
    mov ecx, 0x100000;//1MB大小,之后每次清空4字节，共4MB
    mov esi, KERNEL_PAGE_DIR_ADDR
    mov eax, 0
.clear_kernel_page_dir_entry:
    mov [esi], eax
    add esi, 4
    loop .clear_kernel_page_dir_entry

;开始映射物理低端4MB到虚拟低端4MB
    mov esi, KERNEL_PAGE_DIR_ADDR + 0x1000 ;第一个页表首地址
    mov ecx, 1024 ;映射4MB，一共1024项
    mov eax, 0
    or eax, PT_US_U | PT_RW_RW | PT_P_EXIST
.create_page_table_entry_0_4:
    mov [esi], eax
    add eax, 4096
    add esi, 4
    loop .create_page_table_entry_0_4

;映射页目录和页表 物理地址4MB-8MB
    mov esi, KERNEL_PAGE_DIR_ADDR
    ;先填写属性
    mov eax, KERNEL_PAGE_DIR_ADDR
    add eax, 0x1000 ;下标为0的页表地址
    and eax, 0xFFFFF000;清空属性位
    or eax, PD_US_U | PD_RW_RW | PD_P_EXIST;填写属性,主要这三个就行，其他全0的就行了
    mov ecx,1023

.create_page_dir:
    mov [esi], eax
    add esi,4
    add eax, 0x1000
    loop .create_page_dir

    mov eax, KERNEL_PAGE_DIR_ADDR
    and eax, 0xFFFFF000;
    or eax, PD_US_U | PD_RW_RW | PD_P_EXIST;
    mov [esi], eax ;最后一个页目录项指向自己，这样在虚拟地址空间的映射上，就是最后4MB是页目录和页表


;映射8-16MB到0xc0000000-0xc0800000 代码段 数据段等
    mov esi, KERNEL_PAGE_DIR_ADDR
    add esi, 0x1000
    add esi, 0x300*0x1000 ;定位到0XC0000000对应的页表起点
    mov eax, 0x800000 ;8MB
    and eax, 0xFFFFF000;清空属性位 
    or eax, PD_US_U | PD_RW_RW | PD_P_EXIST;填写属性
    mov ecx, 2048 ;一共2048个表项目

.create_kernel_bin_page_table:
    mov [esi], eax
    add esi,4
    add eax,4096
    loop .create_kernel_bin_page_table

;映射16-24MB到0xFB800000-0xFC000000 当栈用
    mov esi, KERNEL_PAGE_DIR_ADDR
    add esi, 0x1000
    add esi, (KERNEL_FULL_STACK_TOP_ADDR >> 22)*0x1000 ;定位到栈最大时对应的页表起点
    mov eax, 0x1000000 ;16MB
    and eax, 0xFFFFF000;清空属性位 
    or eax, PD_US_U | PD_RW_RW | PD_P_EXIST;填写属性
    mov ecx, 2048 ;一共2048个表项目

.create_kernel_stack_page_table:
    mov [esi], eax
    add esi,4
    add eax,4096
    loop .create_kernel_stack_page_table
    

    ; 目前物理内存布局
    ; 0-1MB boot使用 MBR LOADER使用
    ; 1-4MB 内核文件，内核映像成功展开后这3MB内存可随意使用
    ; 4MB-8MB 内核页目录和页表  映射到虚拟地址为 最后末尾的4MB
    ; 8-16MB 内核展开后程序映像 映射的虚拟地址为3GB-3GB+8MB这段
    ; 16-24MB 内核栈 映射的虚拟地址为 0xFB800000-0xFc000000这段
    popad
    ret

;32位下读取磁盘,还是LBA28方法, 最大能支持到128GB
; 参数:eax:读取数据的起始逻辑扇区号, LBA28模式，只取低28位
;ebx:数据存放的地址
;ecx:读取扇区的数目
read_disk_32:
    pusha
    ; 写入lba28的起始地址
    mov dx, DISK_LBA28_LOW_PORT_ADDR
    out dx, al

    push ecx ;循环移位要用下cx,先放到内存
    mov cx, 8
    shr eax, cl
    mov dx, DISK_LBA28_MID_PORT_ADDR
    out dx, al

    shr eax,cl
    mov dx, DISK_LBA28_HIGH_PORT_ADDR
    out dx, al

    shr eax,cl
    and al, 0x0f; 写入的最后4位地址
    ;顺便写入高4位的其它位配置
    or al, 0xe0; 指定主盘和LBA模式
    mov dx, DISK_DEVICE_PORT_ADDR
    out dx,al
    pop ecx

    ;要读取的扇区起始地址写完，开始准备发送读命令
    mov dx, DISK_COMMAND_PORT_ADDR
    mov al, DISK_COMMAND_READ
    out dx, al

;等待磁盘准备好数据
.wait_data:
    nop
    in al,dx
    and al, 0x88; 第4位为1，表示能读取数据，第8位为1表示硬盘繁忙
    cmp al, 0x08
    jnz .wait_data

    ;到这里数据已经准备好了可以开始读取
    ;计算读取的次数,可以一次读两个字节
    mov eax, ecx
    mov edx, 256
    mul edx
    mov ecx, eax
    ;开始循环读取数据，并放入指定位置
    mov dx, DISK_DATA_PORT_ADDR

.read_data:
    in ax, dx
    mov [ebx], ax
    add ebx, 2
    loop .read_data
;返回调用
    popa
    ret

;将内核文件的节展开成程序在内存中可执行的映像格式
kernel_init:
    pushad
    xor eax, eax
    xor ebx, ebx		;ebx记录程序头表地址
    xor ecx, ecx		;cx记录程序头表中的program header数量
    xor edx, edx		;dx 记录program header尺寸,即e_phentsize

    mov dx, [KERNEL_BIN_BASE_ADDR + 42] ;e_phentsize,表示program header里每个项目的大小
    mov ebx, [KERNEL_BIN_BASE_ADDR + 28]   ; e_phoff,表示第1 个program header在文件中的偏移量
    add ebx, KERNEL_BIN_BASE_ADDR ;加上在内存的起始地址
    mov cx, [KERNEL_BIN_BASE_ADDR + 44]    ;e_phnum,表示有几个program header

;开始处理每个程序头
.process_segment:
    mov eax, [ebx] ;取e_type，判断当前segment类型
    cmp eax, PT_NULL
    je .skip_PT_NULL ;相等就跳过
    ;以下是处理需要载入的内存类型
    ;目前ebx,ecx,edx都保存了有用的值, 使用时尽量要备份，小心被覆盖了找不回来
    mov eax, [ebx + 4] ;p_offset, segment在文件内的偏移
    add eax, KERNEL_BIN_BASE_ADDR ;加上内存地址
    mov esi, eax

    mov eax, [ebx + 8] ;p_vaddr segment需要存放的虚拟内存地址
    mov edi, eax
    cld ;正向复制
    mov eax, ecx ;保存下当前的ecx值
    mov ecx, [ebx + 16] ;e_filesz,该段在文件内的大小
    rep movsb
    mov ecx, eax
    
.skip_PT_NULL:
    add ebx, edx ;edx存的是每个项目的大小, 加上使得ebx指向下一个项的开头
    loop .process_segment

    popad
    ret


