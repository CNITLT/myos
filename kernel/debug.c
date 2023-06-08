#include "debug.h"
#include "print.h"
#include "interrupt.h"
void panic_spin(const char* filename, int line, const char* func, const char* condition){
    close_interrupt();
    put_str("assert error:\n");

    put_str("filename:");
    put_str((char *)filename);
    put_str("\n");

    put_str("line:");
    put_int(line);
    put_str("\n");

    put_str("func:");
    put_str((char*)func); 
    put_str("\n");

    put_str("condition:");
    put_str((char*)condition); 
    put_str("\n");
    while(1);
}