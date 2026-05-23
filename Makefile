dd=dd of=./c.img bs=512 conv=notrunc
AS = nasm
ASFLAGS = -I./boot/include -I./boot
CC = gcc
# -fno-omit-frame-pointer 强制生成push ebp, mov ebp esp两条指令
CFLAGS = -I./device -I./kernel -I./lib -I./lib/kernel -I./lib/user -I./thread -I./userprog -I./fileSystem -I./shell -nostdinc -nostdlib -m32 -c -fno-builtin -fno-omit-frame-pointer
LD = ld
ENTRY_POINT = 0xC0002000
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main 
BUILD_DIR=./build

OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,\
$(notdir $(wildcard ./kernel/*.c ./device/*.c ./lib/*.c ./lib/kernel/*.c ./lib/user/*.c ./thread/*.c ./userprog/*.c ./fileSystem/*.c ./shell/*.c)))


.PHONY:all
all:make_build_dir $(BUILD_DIR)/mbr.bin $(BUILD_DIR)/loader.bin $(BUILD_DIR)/kernel.bin
	$(dd) if=$(BUILD_DIR)/mbr.bin count=1
	$(dd) if=$(BUILD_DIR)/loader.bin count=4 seek=2
	$(dd) if=$(BUILD_DIR)/kernel.bin count=400 seek=9
	readelf -h $(BUILD_DIR)/kernel.bin | grep 入口点
	rm -rf c.img.lock
	cd ./command && make && cd ..
	cd build && objdump -S kernel.bin > kernel.asm && cd ..

	
.PHONY:make_build_dir
make_build_dir:
	if [[ ! -d $(BUILD_DIR) ]];then mkdir $(BUILD_DIR);fi

$(BUILD_DIR)/kernel.bin:$(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

#---------kernel-----------
$(BUILD_DIR)/%.o:./kernel/%.c 
	$(CC) $(CFLAGS) $< -o $@ 

#---------------boot-----------
$(BUILD_DIR)/mbr.bin:./boot/mbr.asm 
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/loader.bin:./boot/loader.asm 
	$(AS) $(ASFLAGS) $< -o $@

#-----------lib--------------
$(BUILD_DIR)/%.o:./lib/kernel/%.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o:./lib/user/%.c
	$(CC) $(CFLAGS) $< -o $@

#----------device--------
$(BUILD_DIR)/%.o:./device/%.c 
	$(CC) $(CFLAGS) $< -o $@

#----------thread--------
$(BUILD_DIR)/%.o:./thread/%.c 
	$(CC) $(CFLAGS) $< -o $@	

#----------userprog--------
$(BUILD_DIR)/%.o:./userprog/%.c 
	$(CC) $(CFLAGS) $< -o $@	

#----------fileSystem--------
$(BUILD_DIR)/%.o:./fileSystem/%.c 
	$(CC) $(CFLAGS) $< -o $@	

#----------shell--------
$(BUILD_DIR)/%.o:./shell/%.c 
	$(CC) $(CFLAGS) $< -o $@	

.PHONY:clean
clean:
	rm -rf ./build/*

.PHONY:test
test:
	echo $(OBJS)
