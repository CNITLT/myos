#ifndef __DEBUG_H
#define __DEBUG_H


/*
@brief 断言失败，打印信息使用
*/
void panic_spin(const char* filename, int line, const char* func, const char* condition);

#define PANIC(...) panic_spin(__FILE__, __LINE__, __func__, ##__VA_ARGS__);

#ifdef NDEBUG
#define assert(CONDITION) ((void)0)
#else
#define assert(CONDITION) if(CONDITION){} \
else{ \
    PANIC(#CONDITION); \
}
#endif

#endif