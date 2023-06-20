#ifndef __KERNEL_PAGE_H
#define __KERNEL_PAGE_H
#include "stdint.h"
#define PAGE_SIZE 4096

//带VALUE的是指用单个成员判断时候的值，而不是用比特位来判断
#define PAGE_P_VALUE_EXIST 1
#define PAGE_P_VALUE_UNEXIST 0
#define PAGE_RW_VALUE_RW 1
#define PAGE_RW_VALUE_R 0
#define PAGE_US_VALUE_SYS 0
#define PAGE_US_VALUE_USER 1

//页属性定义
typedef struct page{
    uint32_t P:1; //P 为1表示该项存在于内存
    uint32_t RW:1; //RW 1为可读可写， 0为可读
    uint32_t US:1; //US 如果为0，则只允许0,1,2特权级访问，1的话任意级别都可以访问
    uint32_t PWT:1;//透写，如果置1，则同时更改高速缓存和内存的数据,同时要在CR3里开启PWT，该位才有效
    uint32_t PCD:1;//Page-level Cache Disable 置1允许该页被加载到高速缓存, 同时要在CR3里开启PCD，该位才有效
    uint32_t A:1;;//页被CPU访问过，则该位被CPU置1
    uint32_t D:1;//D 脏页，如果写了数据，则D位被CPU自动置1，对页目录无效
    uint32_t PAT:1;//PAT 允许更细粒度的设置页面属性，这里置0，不管这个
    uint32_t G:1;//全局页，若为1则一直在TLB高速缓存里，一般是内核的页表，对于用户进程都一样，可以常驻TLB
    uint32_t AVL:3;//硬件保留不用，软件可以自定义
    uint32_t PADDR:20;//物理地址
}page;
typedef page* page_vaddr_t;
typedef page* page_paddr_t;

/*
@brief 在页目录和页面在逻辑上汇集在4MB内的条件下，根据虚拟地址获取到对应的页表项虚拟地址
@param vaddr:vaddr_t: 虚拟地址
@param page_dir:vaddr_t:页目录的虚拟地址
@return page_vaddr_t 对应的页表项首地址
*/
page_vaddr_t get_page_table_entry_vaddr(vaddr_t vaddr, vaddr_t page_dir);

/*
@brief 在页目录和页面在逻辑上汇集在4MB内的条件下,根据虚拟地址获取到对应的页目录表项的虚拟地址
@param vaddr:vaddr_t: 虚拟地址
@param page_dir:vaddr_t:页目录的虚拟地址
@return page_vaddr_t 对应的页表项首地址
*/
page_vaddr_t get_page_dir_entry_vaddr(vaddr_t vaddr, vaddr_t page_dir);


/*
@brief 将虚拟地址转化为对应的物理地址
@param vaddr:vaddr_t: 虚拟地址
@param page_dir:vaddr_t: 页目录
@return paddr_t 转化后的物理地址，如果对应页不存在则返回NULL
*/
paddr_t vaddr2paddr(vaddr_t vaddr, vaddr_t page_dir);

//add_page_dir_entry(vaddr_t addr);

//add_page_table_entry();



#endif