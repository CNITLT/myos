#include "print.h"
#include "io.h"

int main(){
    clear_screen();
    set_cursor_loc(0);
    int j = 0;
    while(1){
        put_int(j++);
        put_char('\n');
    }
    return 0;
}
