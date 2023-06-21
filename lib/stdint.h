#ifndef __LIB_STDINT_H
#define __LIB_STDINT_H
typedef signed char int8_t;
typedef unsigned char uint8_t;

typedef signed short int int16_t;
typedef unsigned short int uint16_t;

typedef signed int int32_t;
typedef unsigned int uint32_t;

typedef signed long long int int64_t;
typedef unsigned long long int uint64_t;

typedef uint64_t size_t;

typedef void* vaddr_t;
typedef void* paddr_t;
typedef void* addr_t;
typedef uint32_t uintaddr_t;
typedef uint32_t uintptr_t;
typedef int32_t error_code_t;

//无错误返回0，固定，具体错误码自定义
#define NOERROR 0
#endif