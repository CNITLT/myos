#ifndef __LIB_STDDEF_H
#define __LIB_STDDEF_H
#define NULL ((void*)0)
#define NAKEDFUNC __attribute__((naked))
#define true (1)
#define false (0)
#define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / (STEP))

#define MAX(a, b) (a > b? a : b)
#define MIN(a, b) (a < b? a : b)

#endif