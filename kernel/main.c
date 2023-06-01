#include "print.h"
#include "io.h"
#include "init.h"
#define __str(x) #x
#define str(x) __str(x)
#define TestInt(num) case num:asm(str(int $##num));break;

int main(){
    clear_screen();
    set_cursor_loc(0);
    init_all();
    open_interrupt();
    while(1){
       
        
    }
    return 0;
}
