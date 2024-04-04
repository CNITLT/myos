#include "keyboard.h"
#include "print.h"
#include "queue.h"
#define KEYBOARD_INTERUPT_NUM 0x21
#define KEYBOARD_BUFF_PORT 0x60

/* 用转义字符定义部分控制字符 */
#define esc		'\033'	 // 八进制表示字符,也可以用十六进制'\x1b'
#define backspace	'\b'
#define tab		'\t'
#define enter		'\n'
#define delete		'\177'	 // 八进制表示字符,十六进制为'\x7f'

/* 以上不可见字符一律定义为0 */
#define char_invisible	0
#define ctrl_l_char	char_invisible
#define ctrl_r_char	char_invisible
#define shift_l_char	char_invisible
#define shift_r_char	char_invisible
#define alt_l_char	char_invisible
#define alt_r_char	char_invisible
#define caps_lock_char	char_invisible

/* 定义控制字符的通码和断码 */
#define shift_l_make	0x2a
#define shift_r_make 	0x36 
#define alt_l_make   	0x38
#define alt_r_make   	0xe038
#define alt_r_break   	0xe0b8
#define ctrl_l_make  	0x1d
#define ctrl_r_make  	0xe01d
#define ctrl_r_break 	0xe09d
#define caps_lock_make 	0x3a

/* 定义以下变量记录相应键是否按下的状态,
 * ext_scancode用于记录makecode是否以0xe0开头 */
static bool ctrl_status, shift_status, alt_status, caps_lock_status, ext_scancode;
static struct IO_Queue io_queue;
/* 以通码make_code为索引的二维数组 */
static char keymap[][2] = {
/* 扫描码   未与shift组合  与shift组合*/
/* ---------------------------------- */
/* 0x00 */	{0,	0},		
/* 0x01 */	{esc,	esc},		
/* 0x02 */	{'1',	'!'},		
/* 0x03 */	{'2',	'@'},		
/* 0x04 */	{'3',	'#'},		
/* 0x05 */	{'4',	'$'},		
/* 0x06 */	{'5',	'%'},		
/* 0x07 */	{'6',	'^'},		
/* 0x08 */	{'7',	'&'},		
/* 0x09 */	{'8',	'*'},		
/* 0x0A */	{'9',	'('},		
/* 0x0B */	{'0',	')'},		
/* 0x0C */	{'-',	'_'},		
/* 0x0D */	{'=',	'+'},		
/* 0x0E */	{backspace, backspace},	
/* 0x0F */	{tab,	tab},		
/* 0x10 */	{'q',	'Q'},		
/* 0x11 */	{'w',	'W'},		
/* 0x12 */	{'e',	'E'},		
/* 0x13 */	{'r',	'R'},		
/* 0x14 */	{'t',	'T'},		
/* 0x15 */	{'y',	'Y'},		
/* 0x16 */	{'u',	'U'},		
/* 0x17 */	{'i',	'I'},		
/* 0x18 */	{'o',	'O'},		
/* 0x19 */	{'p',	'P'},		
/* 0x1A */	{'[',	'{'},		
/* 0x1B */	{']',	'}'},		
/* 0x1C */	{enter,  enter},
/* 0x1D */	{ctrl_l_char, ctrl_l_char},
/* 0x1E */	{'a',	'A'},		
/* 0x1F */	{'s',	'S'},		
/* 0x20 */	{'d',	'D'},		
/* 0x21 */	{'f',	'F'},		
/* 0x22 */	{'g',	'G'},		
/* 0x23 */	{'h',	'H'},		
/* 0x24 */	{'j',	'J'},		
/* 0x25 */	{'k',	'K'},		
/* 0x26 */	{'l',	'L'},		
/* 0x27 */	{';',	':'},		
/* 0x28 */	{'\'',	'"'},		
/* 0x29 */	{'`',	'~'},		
/* 0x2A */	{shift_l_char, shift_l_char},	
/* 0x2B */	{'\\',	'|'},		
/* 0x2C */	{'z',	'Z'},		
/* 0x2D */	{'x',	'X'},		
/* 0x2E */	{'c',	'C'},		
/* 0x2F */	{'v',	'V'},		
/* 0x30 */	{'b',	'B'},		
/* 0x31 */	{'n',	'N'},		
/* 0x32 */	{'m',	'M'},		
/* 0x33 */	{',',	'<'},		
/* 0x34 */	{'.',	'>'},		
/* 0x35 */	{'/',	'?'},
/* 0x36	*/	{shift_r_char, shift_r_char},	
/* 0x37 */	{'*',	'*'},    	
/* 0x38 */	{alt_l_char, alt_l_char},
/* 0x39 */	{' ',	' '},		
/* 0x3A */	{caps_lock_char, caps_lock_char}
/*其它按键暂不处理*/
};


void keyboard_interupt(void){
   uint16_t scan_code = inb(KEYBOARD_BUFF_PORT);
    
   //等于e0说明剩下的才是能判断是哪个按键的依据
    if(scan_code == 0xe0){
        ext_scancode = true;
        return;
    }

    if(ext_scancode){
        scan_code |= 0xe000;
        ext_scancode = false; 
    }
    
    uint16_t make_code = scan_code&0xff7f;//转化为通码来判断按键用来节省判断分支
    bool break_flag = ((scan_code>>7 & 0x1) == 1);//判断是断码还是通码
    // sync_printf("scan_code:%x, break:%d\n",scan_code,break_flag);
    char ascii_code = 0;
    if(break_flag){
        //断码
        //非alt shift ctrl的断码都不用管
        //这里只用修改几个控制状态即可
        if(ctrl_l_make == make_code || ctrl_r_make == make_code) {
            ctrl_status = false;
        }
        else if(shift_l_make == make_code || shift_r_make == make_code){
            shift_status = false;
        }
        else if(alt_l_make == make_code || alt_r_make == make_code){
            alt_status = false;
        }
        return;
    }
    else{
        bool shift = caps_lock_status ^ shift_status;
        //先处理控制字符
        if(ctrl_l_make == make_code || ctrl_r_make == make_code) {
            ctrl_status = true;
        }
        else if(shift_l_make == make_code || shift_r_make == make_code){
            shift_status = true;
        }
        else if(alt_l_make == make_code || alt_r_make == make_code){
            alt_status = true;
        }
        else if(caps_lock_make == make_code){
            caps_lock_status = !caps_lock_status;
        }
        //定义的数组内的部分
        else if(0x0 <= make_code && make_code <= 0x3a){
            uint8_t index = (make_code &= 0x00ff);
           ascii_code =  keymap[index][shift];
        }
        if(ascii_code){
            lock(&io_queue.mutex);
            if(IO_Queue_is_full(&io_queue)){
                unlock(&io_queue.mutex);//已满就不加入直接返回
                return;
            }
            unlock(&io_queue.mutex);
            IO_Queue_push(&io_queue, ascii_code);
            /*
            for(int i = 0; i < io_queue.size; i++){
                sync_printf("%c", io_queue.buff[io_queue.start+i]);
            }
            sync_printf("\n");
            */
        }
    } 

}


void keyboard_init(){
    register_interrupt_func(KEYBOARD_INTERUPT_NUM, keyboard_interupt);
    IO_Queue_init(&io_queue);
}


char read_ascii_from_keyboard_ioqueue(){
    char c = IO_Queue_pop(&io_queue);
    return c;
}