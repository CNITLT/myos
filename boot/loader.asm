%include "include/boot.inc"

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
    ;创建页目录和页表
    call setup_kernel_page_dir_and_table
    ;准备开启分页机制


    


loop_end:
   jmp loop_end




setup_kernel_page_dir_and_table:
    pushad
    ;创建页目录和页表
    ;虚拟地址3GB往上1mb全映射到低端1MB，虚拟地址低端1MB映射到物理地址低端1MB
    ;先清空页目录和页表所在的整个4MB空间
    mov ecx, 0x100000;//1MB大小,之后每次清空4字节，共4MB
    mov esi, KERNEL_PAGE_DIR_ADDR
    mov eax, 0
.clear_kernel_page_dir_entry:
    mov [esi], eax
    inc esi
    loop .clear_kernel_page_dir_entry

    ;创建页目录
    mov esi, KERNEL_PAGE_DIR_ADDR
    ;先填写属性
    mov eax, KERNEL_PAGE_DIR_ADDR
    add eax, 0x1000 ;下标为0的页表地址
    and eax, 0xFFFFF000;清空属性位
    or eax, PD_US_U | PD_RW_RW | PD_P_EXIST;填写属性,主要这三个就行，其他全0的就行了
    mov [esi], eax
    mov [esi + 0xC00], eax
    sub eax, 0x1000 ;将地址指向页目录本身
    mov [esi + 4092], eax ;最后一个页目录项指向自己，这样在虚拟地址空间的映射上，就是最后4MB是页目录和页表

    ;虚拟地址3GB网上对应的页目录项先填写好对应的值，指向对应的页表，虽然这些页表存在，但里面的值都是0， 从0XC01开始，0xC00已经有值了
    mov esi, KERNEL_PAGE_DIR_ADDR + 0xC01 * 4 ;下标0xC01的表项
    mov eax, KERNEL_PAGE_DIR_ADDR + 0x2000 ;下标1的页表地址 KERNEL_PAGE_DIR_ADDR 页目录地址 KERNEL_PAGE_DIR_ADDR+0x1000下标0的页标地址
    or eax, PD_US_U | PD_RW_RW | PD_P_EXIST ;属性
    mov ecx, 254 ;一共256个，0xC00和最后一个指向页目录自己的已经有值了，所以只剩254个
.create_other_kernel_page_dir_entry:
    mov [esi], eax
    add esi, 4
    add eax, 0x1000
    loop .create_other_kernel_page_dir_entry



    ;创建页表, 直接映射4MB了，多一点算了
    mov esi, KERNEL_PAGE_DIR_ADDR + 0x1000 ;第一个页表首地址
    mov ecx, 1024 ;映射4MB，一共1024项
    mov eax, 0
    or eax, PT_US_U | PT_RW_RW | PT_P_EXIST
.create_page_table_entry:
    mov [esi], eax
    add eax, 4096
    add esi, 4
    loop .create_page_table_entry
    ; 目前物理内存布局
    ; 0-4MB 内核使用
    ; 4MB-8MB 内核页目录和页表
    popad
    ret