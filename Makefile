dd=dd of=./c.img bs=512 conv=notrunc
AS = nasm
ASFLAGS = -I./boot/include -I./boot
CC = gcc
CFLAGS = -I./device -I./lib -I./lib/kernel -I./kernel -nostdinc -nostdlib -m32 -c -fno-builtin  
LD = ld
ENTRY_POINT = 0xC0002000
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main 
BUILD_DIR=./build

OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,\
$(notdir $(wildcard ./kernel/*.c ./device/*.c ./lib/*.c ./lib/kernel/*.c)))


.PHONY:all
all:make_build_dir $(BUILD_DIR)/mbr.bin $(BUILD_DIR)/loader.bin $(BUILD_DIR)/kernel.bin
	$(dd) if=$(BUILD_DIR)/mbr.bin count=1
	$(dd) if=$(BUILD_DIR)/loader.bin count=4 seek=2
	$(dd) if=$(BUILD_DIR)/kernel.bin count=200 seek=9
	readelf -h $(BUILD_DIR)/kernel.bin | grep 入口点
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

#----------device--------
$(BUILD_DIR)/%.o:./device/%.c 
	$(CC) $(CFLAGS) $< -o $@

.PHONY:clean
clean:
	rm -rf ./build/*

.PHONY:test
test:
	echo $(OBJS)
