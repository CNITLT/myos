#ifndef __KERNEL_INIT_H
#define __KERNEL_INIT_H


void init_all();

//有些初始化依赖其他机制得放在main_thread里
void init_other_in_main_thread();
#endif