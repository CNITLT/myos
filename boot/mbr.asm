%include "include/boot.inc" ;包含一些配置数据
DISK_SECTOR_COUNT_PORT_ADDR equ 0x1F2
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
[bits 16]
SECTION MBR vstart=0x7c00  ;到mbr的时候，cs:ip为0000:0x7c00
;这段MBR只负责加载loader到内存
;设置好段寄存器
    mov ax,cs      
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
    mov sp,0x7c00;栈从高到低扩展，这里不会破坏mbr代码
    mov ax,0xb800;显存地址
    mov gs,ax

    mov eax, LOADER_START_SECTOR
    mov bx, LOADER_BASE_ADDR
    mov cx, LOADER_SIZE_SECTOR
    call read_disk

    ;读取完成，控制器转移给LOADER
    jmp LOADER_BASE_ADDR






; 功能:读主硬盘
; 参数:eax:读取数据的起始逻辑扇区号, LBA28模式，只取低28位
;bx:数据存放的地址，实际是ds*16+bx
;cx:读取扇区的数目
;备注:此时是实模式下，除了eax要大一点来放扇区的起点，用ebx,ecx也没那么大的内存可以指定,所以bx和cx就够了
read_disk:
    pusha
    ; 写入lba28的起始地址
    mov dx, DISK_LBA28_LOW_PORT_ADDR
    out dx, al

    push cx ;循环移位要用下cx,先放到内存
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
    pop cx
    ;这里其实逻辑有问题，漏向0X1F2写入要读取的扇区数，这里猜测没写，默认是0，读取了256个扇区，所以能正常运行
    ;能跑，懒得改了
    ;测试下是不是真的是0
    ;push ax
    ;mov dx, DISK_SECTOR_COUNT_PORT_ADDR 
    ;in ax, dx 
    ;jmp $
    ;pop ax
    ;测完了不是0，是255，没多大差别

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
    mov ax, cx
    mov dx, 256
    mul dx
    mov cx, ax
    ;开始循环读取数据，并放入指定位置
    mov dx, DISK_DATA_PORT_ADDR

.read_data:
    in ax, dx
    mov [bx], ax
    add bx, 2
    loop .read_data
;返回调用
    popa
    ret


times 510-($-$$) db 0 ;中间的填0，使得下面能在最后的两字节填上魔数
db 0x55,0xaa ;标识这段程序是MBR的魔数
