
;GDT各个位属性对应的数字, GDT共64位，分高32位和低32位
;高32位
;------31~24------23---22---21---20-----------19~16---15-14~13--12--11~8--------7~0------
;| 段基址(31~24) | G | D/B | L | AVL | 段界限(19~16) | P | DPL | S | TYPE | 段基址(23~16) |
;----------------------------------------------------------------------------------------
;低32位
;---------------31~16--------------------------------------------15~0--------------------
;|             段基址(15~0)                |                   段界限(15~0)              |
;----------------------------------------------------------------------------------------
;高32位部分
;G位, 指定界限粒度是1字节还是4kB

GDT_G_4K    equ 0x800000
GDT_G_1Byte equ 0x000000
;DB位, 
;对代码段来说如果为1，就用EIP寻址，且指令和操作数当32位处理，如果为0就用IP寻址，指令和操作数当16位处理
;对栈段来说，如果为1，就用ESP当栈顶，如果为0就用SP当栈顶

GDT_D_CODE_32 equ 0x400000
GDT_D_CODE_16 equ 0x000000


GDT_B_STACK_32 equ 0x400000
GDT_B_STACK_16 equ 0x000000

;DB汇总起来是一样的，开启32位的话，代码和栈都要设置为1

GDT_DB_32 equ 0x400000
GDT_DB_16 equ 0x000000

;L位
;为1是64位代码段,0是32位
GDT_L_64 equ 0x200000
GDT_L_32 equ 0x000000

;AVL位 保留

;P位 为1 表示该描述符存在，为0不存在
GDT_P_EXIST equ 0x8000
GDT_P_UNEXIST equ 0x0000


;DPL位，特权级
GDT_DPL_0 equ 0x0000
GDT_DPL_1 equ 0x1000
GDT_DPL_2 equ 0x2000
GDT_DPL_3 equ 0x3000

;S位 为0为系统段，为1为数据段. 这里的系统其实是指CPU硬件本身所需要的数据结构，而不是软件层面的操作系统, 软件层面的在这里都被当成数据段
;需要与type字段联合起来解读实际含义
GDT_S_SYSTEM equ 0x0000
GDT_S_DATA equ 0x1000

;type字段, 以下只定义了x86要用的，兼容以前的没写
;s为0的系统段
GDT_TYPE_LDT equ 0x200
GDT_TYPE_TASK_GATE equ 0x500
GDT_TYPE_TSS equ 0x900
GDT_TYPE_CALL_GATE equ 0xC00
GDT_TYPE_INTERRUPT_GATE equ 0xE00
GDT_TYPE_TRAP_GATE equ 0xF00


;s为1的数据段
GDT_TYPE_CODE equ 0x800
GDT_TYPE_DATA equ 0x000

;s为1的数据段属性
GDT_TYPE_CODE_R equ 0x200 ;代码段可读
GDT_TYPE_CODE_CONFORMING equ 0x400 ;一致性代码段
GDT_TYPE_DATA_W equ 0x200 ;数据段可写
GDT_TYPE_DATA_EXTEND_DOWN equ 0x400 ;向下扩展
GDT_TYPE_DATA_EXTEND_UP equ 0x000 ;向上扩展


;同时设置S和TYPE防止弄混
;s为0的系统段
GDT_S_TYPE_LDT equ GDT_TYPE_LDT | GDT_S_SYSTEM
GDT_S_TYPE_TASK_GATE equ GDT_TYPE_TASK_GATE | GDT_S_SYSTEM
GDT_S_TYPE_TSS equ GDT_TYPE_TSS | GDT_S_SYSTEM
GDT_S_TYPE_CALL_GATE equ GDT_TYPE_CALL_GATE | GDT_S_SYSTEM
GDT_S_TYPE_INTERRUPT_GATE equ GDT_TYPE_INTERRUPT_GATE | GDT_S_SYSTEM
GDT_S_TYPE_TRAP_GATE equ GDT_TYPE_TRAP_GATE | GDT_S_SYSTEM

;s为1的数据段
GDT_S_TYPE_CODE equ GDT_TYPE_CODE | GDT_S_DATA
GDT_S_TYPE_DATA equ GDT_TYPE_DATA | GDT_S_DATA


;---------------15~3-------------------2---1~0---
;|              Index                | TI | RPL |             
;-----------------------------------------------
;定义选择子里的属性
RPL_0  equ   0x0
RPL_1  equ   0x1
RPL_2  equ   0x2
RPL_3  equ   0x3
TI_GDT	 equ   0x0
TI_LDT	 equ   0x4
