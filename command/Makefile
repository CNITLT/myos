CC = gcc
# -fno-omit-frame-pointer 强制生成push ebp, mov ebp esp两条指令
CFLAGS = -I../lib -I../lib/kernel -nostdinc -nostdlib -m32 -c -fno-builtin -fno-omit-frame-pointer -ffunction-sections -fdata-sections
LD = ld
LDFLAGS = -m elf_i386 -e prog --gc-sections
BUILD_DIR=../build
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,\
$(notdir $(wildcard ../kernel/*.c ../device/*.c ../lib/*.c ../lib/kernel/*.c ../lib/user/*.c ../thread/*.c ../userprog/*.c ../fileSystem/*.c ../shell/*.c)))

dd=dd of=../c.img bs=512 conv=notrunc
all:
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/prog.o ./prog.c
	$(LD) $(LDFLAGS) $(OBJS) $(BUILD_DIR)/prog.o -o  $(BUILD_DIR)/prog.bin
	$(dd) if=$(BUILD_DIR)/prog.bin count=400 seek=500
