#include "print.h"
#include "io.h"
#include "init.h"
#include "debug.h"
#include "string.h"
#define __str(x) #x
#define str(x) __str(x)
#define TestInt(num) case num:asm(str(int $##num));break;
void timer_interrupt(void){
    static int count = 0;
    count++;
    put_str("this is timer interrupt func count:");
    put_int(count);
    put_char('\n');
}
int main(){
    clear_screen();
    set_cursor_loc(0);
    init_all();
    register_interrupt_func(0x20, timer_interrupt);
   
    //open_interrupt();
    close_interrupt();

    char *str = "\n123456\n";
    char strarr[100];
    //put_hex(strarr);
    //put_char('\n');
    //put_hex(str);
    memset(strarr, 0, 100);
    memcpy(strarr, str, strlen(str)+1);
    put_int(strlen(strarr));
    //put_str(strarr);
    strcat(strarr, str);
    put_str("strcat:\n");
    put_int(strlen(strarr)); 
    put_str(strarr);
    put_int(strchrs(strarr, '\n'));
    while(1){ 
    }
    return 0;
}
