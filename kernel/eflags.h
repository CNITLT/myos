#ifndef __EFLAGS_H
#define __EFLAGS_H
//---------------    eflags属性    ---------------- 

/********************************************************
--------------------------------------------------------------
		  Intel 8086 Eflags Register
--------------------------------------------------------------
*
*     15|14|13|12|11|10|F|E|D C|B|A|9|8|7|6|5|4|3|2|1|0|
*      |  |  |  |  |  | | |  |  | | | | | | | | | | | '---  CF……Carry Flag
*      |  |  |  |  |  | | |  |  | | | | | | | | | | '---  1 MBS
*      |  |  |  |  |  | | |  |  | | | | | | | | | '---  PF……Parity Flag
*      |  |  |  |  |  | | |  |  | | | | | | | | '---  0
*      |  |  |  |  |  | | |  |  | | | | | | | '---  AF……Auxiliary Flag
*      |  |  |  |  |  | | |  |  | | | | | | '---  0
*      |  |  |  |  |  | | |  |  | | | | | '---  ZF……Zero Flag
*      |  |  |  |  |  | | |  |  | | | | '---  SF……Sign Flag
*      |  |  |  |  |  | | |  |  | | | '---  TF……Trap Flag
*      |  |  |  |  |  | | |  |  | | '---  IF……Interrupt Flag
*      |  |  |  |  |  | | |  |  | '---  DF……Direction Flag
*      |  |  |  |  |  | | |  |  '---  OF……Overflow flag
*      |  |  |  |  |  | | |  '----  IOPL……I/O Privilege Level
*      |  |  |  |  |  | | '-----  NT……Nested Task Flag
*      |  |  |  |  |  | '-----  0
*      |  |  |  |  |  '-----  RF……Resume Flag
*      |  |  |  |  '------  VM……Virtual Mode Flag
*      |  |  |  '-----  AC……Alignment Check
*      |  |  '-----  VIF……Virtual Interrupt Flag  
*      |  '-----  VIP……Virtual Interrupt Pending
*      '-----  ID……ID Flag
*
*
**********************************************************/


#define EFLAGS_MBS	(1 << 1)	// 此项必须要设置 固定为1
#define EFLAGS_IF_1	(1 << 9)	// if为1,开中断
#define EFLAGS_IF_0	0		// if为0,关中断
#define EFLAGS_IOPL_3	(3 << 12)	// IOPL3,用于测试用户程序在非系统调用下进行IO
#define EFLAGS_IOPL_0	(0 << 12)	// IOPL0
#endif