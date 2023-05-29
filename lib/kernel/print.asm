
VGA_TXT_MODE_START_ADDR equ 0xB8000 ;显存文本模式的起始地址
VGA_END_ADDR equ 0xBFFFF ;显存的末尾地址
VGA_CRT_ADDR_REG_PORT equ 0x3D4 ;CRT地址寄存器的端口地址，默认是这个，可能会被其它设置影响
VGA_CRT_DATA_REG_PORT equ 0x3D5 ;CRT数据寄存器的端口地址，默认是这个，可能会被其它设置影响
VGA_CRT_CURSOR_LOW equ 0x0F ;CRT里光标低8位寄存器
VGA_CRT_CURSOR_HIGH equ 0x0E ;CRT里光标高8位寄存器
[bits 32]
;读取光标值
;无参数
;返回值:eax,当前光标位置
;uint32_t read_cursor_loc();
global read_cursor_loc
read_cursor_loc:
    xor eax,eax
    push edx
    ;读取当前光标的位置
    mov dx, VGA_CRT_ADDR_REG_PORT
    mov al, VGA_CRT_CURSOR_HIGH
    out dx, al
    mov dx, VGA_CRT_DATA_REG_PORT
    in al, dx
    mov ah, al

    mov dx, VGA_CRT_ADDR_REG_PORT
    mov al, VGA_CRT_CURSOR_LOW
    out dx, al
    mov dx, VGA_CRT_DATA_REG_PORT
    in al, dx
    pop edx
    ret
;设置光标值, 超过2000不会自动滚屏
;uint32_t:光标新位置值
;无返回值
;等价C语言 void set_cursor_loc(uint32_t pos);
global set_cursor_loc
set_cursor_loc:
    mov eax, [esp + 4] ;获取参数
    push edx
    push ebx
    mov ebx, eax
    xor eax, eax 
    mov dx, VGA_CRT_ADDR_REG_PORT
    mov al, VGA_CRT_CURSOR_LOW
    out dx, al
    mov dx, VGA_CRT_DATA_REG_PORT
    mov al, bl
    out dx, al
  
    mov dx, VGA_CRT_ADDR_REG_PORT
    mov al, VGA_CRT_CURSOR_HIGH
    out dx, al
    mov dx, VGA_CRT_DATA_REG_PORT
    mov al, bh
    out dx, al
    pop ebx
    pop edx
    ret

;滚屏,所有行向上滚动一行,光标本身也会同时设置
;无参数
;无返回值
;void roll_up();
global roll_up
roll_up:
    pushad
    cld
    mov edi, VGA_TXT_MODE_START_ADDR
    mov esi, VGA_TXT_MODE_START_ADDR+160
    mov ecx,  80*25*2/4 ;80*25模式下，一行80个字符，只需要移动19行数据，最后一行清0, 同时一个字符用两个字节表示, 低直接ascii码，高字节设置颜色
    rep movsd;一次移动4字节，所以上面除4

    ;最后一行清零
    mov ecx, 80
.clear_end_line:
    mov word [edi], 0 ;用0填充
    add edi, 2
    loop .clear_end_line
    ;开始设置光标的新值
    call read_cursor_loc
    sub eax, 80
    push eax
    call set_cursor_loc
    pop eax
    popad
    ret