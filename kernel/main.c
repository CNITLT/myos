#include "print.h"
#include "io.h"
#include "interrupt.h"

#define __str(x) #x
#define str(x) __str(x)
#define TestInt(num) case num:asm(str(int $##num));break;

int main(){
    clear_screen();
    set_cursor_loc(0);
    interrupt_init();
    while(1){
    
        for(int i = 0; i < 32; i++){
            put_int(i);
            put_str("-----------\n");
            switch(i){
               TestInt(0);
               TestInt(1);
               TestInt(2);
               TestInt(3);
               TestInt(4);
               TestInt(5);
               TestInt(6);
               TestInt(7);
               TestInt(8);//人为触发的中断不会导致错误码
               TestInt(9);
               TestInt(10);
               TestInt(11);
               TestInt(12);
               TestInt(13);
               TestInt(14);
               TestInt(15);
               TestInt(16);
               TestInt(17);
               TestInt(18);
               TestInt(19);
               TestInt(20);
               TestInt(21);
               TestInt(22);
               TestInt(23);
               TestInt(24);
               TestInt(25);
               TestInt(26);
               TestInt(27);
               TestInt(28);
               TestInt(29);
               TestInt(30);
               TestInt(31);
            }
            
        }
        
    }
    return 0;
}
