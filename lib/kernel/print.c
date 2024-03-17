#include "print.h"
#include "io.h"



vaddr_t get_vga_txt_mode_start_addr(){
    return (vaddr_t)VGA_TXT_MODE_START_ADDR;
}

uint32_t read_cursor_loc(){
    uint32_t loc = 0;
    outb(VGA_CRT_ADDR_REG_PORT,VGA_CRT_CURSOR_HIGH);
    loc = inb(VGA_CRT_DATA_REG_PORT);
    loc = loc << 8;
    
    outb(VGA_CRT_ADDR_REG_PORT,VGA_CRT_CURSOR_LOW);
    loc += inb(VGA_CRT_DATA_REG_PORT); 
    return loc;
}
void set_cursor_loc(uint32_t pos){
    outb(VGA_CRT_ADDR_REG_PORT,VGA_CRT_CURSOR_LOW);
    outb(VGA_CRT_DATA_REG_PORT,pos & 0xFF);
    outb(VGA_CRT_ADDR_REG_PORT,VGA_CRT_CURSOR_HIGH);
    outb(VGA_CRT_DATA_REG_PORT,(pos & 0xFF00) >> 8); 
}
void roll_up(){
    char (*screen)[80][2] = get_vga_txt_mode_start_addr();
    for(int row = 0; row < 24; row++){
        for(int col = 0; col < 80; col++){
            screen[row][col][0] = screen[row+1][col][0];
            screen[row][col][1] = screen[row+1][col][1];  
        }
    }
    for(int col = 0; col < 80; col++){
        screen[24][col][0] = 0;
        screen[24][col][1] = 0;  
    } 
    set_cursor_loc(read_cursor_loc() - 80);
}

void put_char(char ch){
    char (*screen)[80][2] = get_vga_txt_mode_start_addr();//显存其实地址,[x][y][0] x行，y列，显示的字符 [x][y][1]x行，y列，显示的颜色
    uint32_t cursor_loc = read_cursor_loc();
    uint32_t row = cursor_loc / 80;
    uint32_t col = cursor_loc % 80;
    if (ch == '\n'){
        if(row == 24){
            roll_up();
            cursor_loc -= 80;
        }
        set_cursor_loc(cursor_loc + 80 - col);    
    }
    else if(ch == '\r'){
        set_cursor_loc(cursor_loc - col);
    }
    else if(ch == '\b'){
        if(1 <= cursor_loc){
            screen[row][col-1][0] = 0;
            set_cursor_loc(cursor_loc - 1);
        }
    }
    else{
        screen[row][col][0] = ch;
        screen[row][col][1] = BLACK_BACKGROUND_WHITE_CHAR;
        cursor_loc++;
        set_cursor_loc(cursor_loc);
        if(cursor_loc > 2000){
            roll_up();
        }
    }
}


void put_str(char* str){
    while(*str != 0){
        put_char(*str);
        str++;
    }
}

void put_int(int32_t num){
    if(num < 0){
        put_char('-');
        num *= -1;
    }
    char stack[10];//10个字节足够存下int32范围的所有位数了
    int i = 0;
    do{
        stack[i] = '0' + num % 10;
        i++;
        num /= 10;
    }while(num != 0);
    for(;i>0; i--){
        put_char(stack[i-1]);
    }
}

void put_hex(uint32_t num){
    uint8_t stack[8];
    int i = 0;
    do{
       stack[i] = num % 16;
       num /= 16;
       i++;
    }while(num != 0);
    for(;i>0;i--){
        uint8_t ch = stack[i-1];
        if(ch <= 9){
            put_char('0' + ch);
        }
        else{
            put_char('A' + ch - 10);
        }
    }
}


void put_hex64(uint64_t num){
    uint8_t stack[16];
    int i = 0;
    do{
       stack[i] = num % 16;
       num /= 16;
       i++;
    }while(num != 0);
    for(;i>0;i--){
        uint8_t ch = stack[i-1];
        if(ch <= 9){
            put_char('0' + ch);
        }
        else{
            put_char('A' + ch - 10);
        }
    }
}


void clear_screen(){
    char (*screen)[80][2] = get_vga_txt_mode_start_addr();
    for(int row = 0; row < 25; row++){
        for(int col = 0; col < 80; col++){
            screen[row][col][0] = 0;
        }
    }
    set_cursor_loc(0);
}