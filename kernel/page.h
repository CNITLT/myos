#ifndef __KERNEL_PAGE_H
#define __KERNEL_PAGE_H
#include "stdint.h"
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

#endif