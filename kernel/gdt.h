#ifndef __GDT_H
#define __GDT_H
#include "stdint.h"
#include "stddef.h"
#define GDT_BASE_VADDR (0x940) //即是物理地址也是虚拟地址，GDT在0-4MB的直接映射区域内
#define GAT_LENGTH (63) //算上第一个不能用的和预留的空位一共有63个空间能用，不过默认第一个不能用，第二个和第三页已经在loader里面用来对应代码段和数据段了


//GDT各个位属性对应的数字, GDT共64位，分高32位和低32位
//高32位
//------31~24------23---22---21---20-----------19~16---15-14~13--12--11~8--------7~0------
//| 段基址(31~24) | G | D/B | L | AVL | 段界限(19~16) | P | DPL | S | TYPE | 段基址(23~16) |
//----------------------------------------------------------------------------------------
//低32位
//---------------31~16--------------------------------------------15~0--------------------
//|             段基址(15~0)                |                   段界限(15~0)              |
//----------------------------------------------------------------------------------------


//高32位部分可用的宏，限用于32位数据下的位操作
//G位, 指定界限粒度是1字节还是4kB

#define GDT_G_4K (0x800000)
#define GDT_G_1Byte  (0x000000)


//DB位, 
//对代码段来说如果为1，就用EIP寻址，且指令和操作数当32位处理，如果为0就用IP寻址，指令和操作数当16位处理
//对栈段来说，如果为1，就用ESP当栈顶，如果为0就用SP当栈顶


#define GDT_D_CODE_32 (0x400000)
#define GDT_D_CODE_16 (0x000000)


#define GDT_B_STACK_32 (0x400000)
#define GDT_B_STACK_16 (0x000000)

//DB汇总起来是一样的，开启32位的话，代码和栈都要设置为1

#define GDT_DB_32 (0x400000)
#define GDT_DB_16 (0x000000)

//L位
//为1是64位代码段,0是32位
#define GDT_L_64 (0x200000)
#define GDT_L_32 (0x000000)

//AVL位 保留

//P位 为1 表示该描述符存在，为0不存在
#define GDT_P_EXIST (0x8000)
#define GDT_P_UNEXIST (0x0000)


//DPL位，特权级
#define GDT_DPL_0 (0x0000)
#define GDT_DPL_1 (0x1000)
#define GDT_DPL_2 (0x2000)
#define GDT_DPL_3 (0x3000)

//S位 为0为系统段，为1为数据段. 这里的系统其实是指CPU硬件本身所需要的数据结构，而不是软件层面的操作系统, 软件层面的在这里都被当成数据段
//需要与type字段联合起来解读实际含义
#define GDT_S_SYSTEM (0x0000)
#define GDT_S_DATA (0x1000)

//type字段, 以下只定义了x86要用的，兼容以前的没写
//s为0的系统段
#define GDT_TYPE_LDT (0x200)
#define GDT_TYPE_TASK_GATE (0x500)
#define GDT_TYPE_TSS (0x900)
#define GDT_TYPE_CALL_GATE (0xC00)
#define GDT_TYPE_INTERRUPT_GATE (0xE00)
#define GDT_TYPE_TRAP_GATE (0xF00)


//s为1的数据段
#define GDT_TYPE_CODE (0x800)
#define GDT_TYPE_DATA (0x000)

//s为1的数据段属性
#define GDT_TYPE_CODE_R (0x200) //代码段可读
#define GDT_TYPE_CODE_CONFORMING (0x400) //一致性代码段
#define GDT_TYPE_DATA_W (0x200) //数据段可写
#define GDT_TYPE_DATA_EXTEND_DOWN (0x400) //向下扩展
#define GDT_TYPE_DATA_EXTEND_UP (0x000) //向上扩展


//同时设置S和TYPE防止弄混
//s为0的系统段
#define GDT_S_TYPE_LDT (GDT_TYPE_LDT | GDT_S_SYSTEAM)
#define GDT_S_TYPE_TASK_GATE (GDT_TYPE_TASK_GATE | GDT_S_SYSTEM)
#define GDT_S_TYPE_TSS (GDT_TYPE_TSS | GDT_S_SYSTEM)
#define GDT_S_TYPE_CALL_GATE (GDT_TYPE_CALL_GATE | GDT_S_SYSTEM)
#define GDT_S_TYPE_INTERRUPT_GATE (GDT_TYPE_INTERRUPT_GATE | GDT_S_SYSTEM)
#define GDT_S_TYPE_TRAP_GATE (GDT_TYPE_TRAP_GATE | GDT_S_SYSTEM)

//s为1的数据段
#define GDT_S_TYPE_CODE (GDT_TYPE_CODE | GDT_S_DATA)
#define GDT_S_TYPE_DATA (GDT_TYPE_DATA | GDT_S_DATA)


//---------------15~3-------------------2---1~0---
//|              Index                | TI | RPL |             
//-----------------------------------------------
//定义选择子里的属性
#define RPL_0 (0x0)
#define RPL_1  (0x1)
#define RPL_2  (0x2)
#define RPL_3  (0x3)
#define TI_GDT	 (0x0)
#define TI_LDT	 (0x4)

// 通过gdt_entry设置使用的宏
#define GDT_ENTRY_S_SYSTEM 0
#define GDT_ENTRY_S_DATA 1

//type字段, 以下只定义了x86要用的，兼容以前的没写
//s为0的系统段
#define GDT_ENTRY_TYPE_LDT (GDT_TYPE_LDT >> 8)
#define GDT_ENTRY_TYPE_TASK_GATE (GDT_TYPE_TASK_GATE >> 8)
#define GDT_ENTRY_TYPE_TSS (GDT_TYPE_TSS >> 8)
#define GDT_ENTRY_TYPE_CALL_GATE (GDT_TYPE_CALL_GATE >> 8)
#define GDT_ENTRY_TYPE_INTERRUPT_GATE (GDT_TYPE_INTERRUPT_GATE >> 8)
#define GDT_ENTRY_TYPE_TRAP_GATE (GDT_TYPE_TRAP_GATE >> 8)


//s为1的数据段
#define GDT_ENTRY_TYPE_CODE (GDT_TYPE_CODE >> 8)
#define GDT_ENTRY_TYPE_DATA (GDT_TYPE_DATA >> 8)

//s为1的数据段属性
#define GDT_ENTRY_TYPE_CODE_R (GDT_TYPE_CODE_R >> 8) //代码段可读
#define GDT_ENTRY_TYPE_CODE_CONFORMING (GDT_TYPE_CODE_CONFORMING >> 8) //一致性代码段
#define GDT_ENTRY_TYPE_DATA_W (GDT_TYPE_DATA_W >> 8) //数据段可写
#define GDT_ENTRY_TYPE_DATA_EXTEND_DOWN (GDT_TYPE_DATA_EXTEND_DOWN >> 8) //向下扩展
#define GDT_ENTRY_TYPE_DATA_EXTEND_UP (GDT_TYPE_DATA_EXTEND_UP >> 8) //向上扩展

//DPL位，特权级
#define GDT_ENTRY_DPL_0 (0x0)
#define GDT_ENTRY_DPL_1 (0x1)
#define GDT_ENTRY_DPL_2 (0x2)
#define GDT_ENTRY_DPL_3 (0x3)


//当前GDT的选择子
#define GDT_SELECTOR_CODE ((0x0001 << 3) | TI_GDT | RPL_0)
#define GDT_SELECTOR_DATA ((0x0002 << 3) | TI_GDT | RPL_0)
#define GDT_SELECTOR_GLOBAL_TSS ((0x0003 << 3) | TI_GDT | RPL_0)
#define GDT_SELECTOR_USER_CODE ((0x0004 << 3) | TI_GDT | RPL_3)
#define GDT_SELECTOR_USER_DATA ((0x0005 << 3) | TI_GDT | RPL_3)


/*
47                 15          0
--------------------------------
|  GDT内存起始地址  |  GDT界限  |
--------------------------------
*/
struct gdt_ptr {
  uint16_t limit; //GDT界限值等于表字节大小减1
  paddr_t base;
} __attribute__((packed));

//GDT各个位属性对应的数字, GDT共64位，分高32位和低32位
//高32位
//------31~24------23---22---21---20-----------19~16---15-14~13--12--11~8--------7~0------
//| 段基址(31~24) | G | D/B | L | AVL | 段界限(19~16) | P | DPL | S | TYPE | 段基址(23~16) |
//----------------------------------------------------------------------------------------
//低32位
//---------------31~16--------------------------------------------15~0--------------------
//|             段基址(15~0)                |                   段界限(15~0)              |
//----------------------------------------------------------------------------------------

struct gdt_entry{
  uint16_t seg_limit_0_15;
  uint16_t seg_base_addr_0_15;
  uint8_t  seg_base_addr_16_23;
  uint8_t type:4;
  uint8_t s:1;
  uint8_t dpl:2;
  uint8_t p:1;
  uint16_t seg_limit_16_19:4;
  uint16_t avl:1;
  uint16_t l:1;
  uint16_t db:1; 
  uint16_t g:1;
  uint16_t seg_base_addr_24_31:8;
} __attribute__((packed)); 


/*
@brief 设置gdt里的seg_base_addr选项
@param p_gdt_entry: struct gdt_entry* :待设置的gdt表项地址
@param addr: addr_t :需要设置的地址数据
*/
void set_gdt_entry_seg_base_addr(struct gdt_entry* p_gdt_entry,addr_t addr);

/*
@brief 获取完整的段地址
@param p_gdt_entry: struct gdt_entry* :gdt表项地址
@return addr_t :段地址数据
*/
addr_t get_gdt_entry_seg_base_addr(struct gdt_entry* p_gdt_entry);

/*
@brief 设置gdt里的seg_base_addr选项
@param p_gdt_entry: struct gdt_entry* :待设置的gdt表项地址
@param seg_limit: uint32_t : 需要设置的段界限值
*/
void set_gdt_entry_seg_limit(struct gdt_entry* p_gdt_entry,uint32_t seg_limit);

/*
@brief 获取完整的段界限
@param p_gdt_entry: struct gdt_entry* :gdt表项地址
@return addr_t :段界限数据
*/
uint32_t get_gdt_entry_seg_limit(struct gdt_entry* p_gdt_entry);

/*
@brief 获取gdt表的首地址
@return vaddr_t: gdt表首的虚拟地址
*/
vaddr_t get_gdt_addr();

/*
@brief 获取gdt的界限值
@return uint16_t: gdt当前界限值
*/
uint16_t get_gdt_limit();

/*
@brief 获取gdt_ptr相关信息
@return struct gdt_ptr: gdt_ptr信息
*/
struct gdt_ptr get_gdt_ptr();
/*
@brief 设置gdt表
@param: p_gdt_ptr:struct gdt_ptr* : 指向gdt_ptr类型的指针
@return: struct gdt_ptr: 旧的gdt_ptr设置
*/
struct gdt_ptr set_gdt(struct gdt_ptr* p_gdt_ptr);


/*
@brief 打印GDT表项目信息
@param p_gdt_entry: struct gdt_entry* :gdt表项地址
*/
void pf_gdt_entry(struct gdt_entry* p_gdt_entry);

/*
@brief 用一个默认的配置初始化gdt表项的部分配置, 4G平坦内存设置，G=1，DB=1,L=0,P=0,DPL S TYPE待设置
@param p_gdt_entry: struct gdt_entry* :gdt表项地址
*/
void init_gdt_entry_part_with_default_config(struct gdt_entry* p_gdt_entry);

/*
@brief 用默认配置初始化gdt表项目，且部分使用参数手配置，配置完后,p=1
@param p_gdt_entry: struct gdt_entry* :gdt表项地址
@param s: uint16_t: s位配置
@param type: uint16_t: type位配置
@param dpl: uint16_t: dpl位配置
*/
void init_gdt_entry_with_default_config_and_param(struct gdt_entry* p_gdt_entry, uint16_t s, uint16_t type, uint16_t dpl);



#endif