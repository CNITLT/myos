%include "include/boot.inc"

SECTION loader vstart=LOADER_BASE_ADDR
[bits 16] ;还没开启保护模式，目前还是16位
jmp .loader_start

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

.loader_start:
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
    jmp dword GDT_SELECTOR_CODE:.pe_mode_start

.pe_mode_start:
    ;刷新其它的选择子
    mov ax, GDT_SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov ss, ax
    mov esp,LOADER_STACK_TOP

    ;创建页表
    ;虚拟地址3GB往上的1mb全映射到低端1MB，虚拟地址低端1MB映射到物理地址低端1MB
    

    jmp .pe_mode_start

    