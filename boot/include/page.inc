

;页目录和页表属性定义, 有些属性的意义对页目录和页表一样，有些意义不同
;-------31~12-------11~9--8----7----6---5----4-----3-----2----1---0--
;| 物理地址31~12位 | AVL | G | PAT | D | A | PCD | PWT | US | RW | P |
;--------------------------------------------------------------------
;页默认是4K所以只要高20位
;AVL硬件不用，软件层面的操作系统可自定义
;以下PD是页目录的属性定义，PT是页表的,值是一样的
;G
;全局页，若为1则一直在TLB高速缓存里，一般是内核的页表，对于用户进程都一样，可以常驻TLB
PD_G_IN_TLB equ 0x100
PT_G_IN_TLB equ PD_G_IN_TLB
;PAT 允许更细粒度的设置页面属性，这里置0，不管这个
PD_PAT_INIT equ 0x00
PT_PAT_INIT equ 0x00

;D 脏页，如果写了数据，则D位被CPU自动置1，对页目录无效
PD_D_INIT equ 0x00
PT_D_INIT equ 0x00

;A 页被CPU访问过，则该位被CPU置1
PD_A_INIT equ 0x00
PT_A_INIT equ 0x00

;PCD  Page-level Cache Disable 置1允许该页被加载到高速缓存, 同时要在CR3里开启PCD，该位才有效
PD_PCD_CAHCE equ 0x10
PT_PCD_CACHE equ 0x10

;PWT 透写，如果置1，则同时更改高速缓存和内存的数据,同时要在CR3里开启PWT，该位才有效
PD_PWT_WT equ 0x8
PT_PWT_WT equ 0x8

;US 如果为0，则只允许0,1,2特权级访问，1的话任意级别都可以访问
PD_US_S equ 0x0
PD_US_U equ 0x4

PT_US_S equ 0x0
PT_US_U equ 0x4

;RW 1为可读可写， 0为可读
PD_RW_RW  equ 0x2
PD_RW_R equ 0x0

PT_RW_RW  equ 0x2
PT_RW_R equ 0x0

;P 为1表示该项存在于内存
PD_P_EXIST equ 0x1
PD_P_UNEXIST equ 0x0

PT_P_EXIST equ 0x1
PT_P_UNEXIST equ 0x0

