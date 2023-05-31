CC=gcc
CFLAGS=-m32 -I../lib/kernel -I../lib -nostdinc -nostdlib
LD32=-m elf_i386

kernel.bin:main.o interrupt.o
	make -C ../lib/kernel all
	ld $(LD32) main.o  interrupt.o ../lib/kernel/print.o  -e main -Ttext 0xC0002000 -o kernel.bin
main.o:main.c
	$(CC) $(CFLAGS) main.c -o main.o -c

interrupt.o:interrupt.h interrupt.c
	$(CC) $(CFLAGS) interrupt.c -c -o interrupt.o
.PHONY:clean
clean:
	-rm -f kernel.bin
	-rm -f main.o
	-rm -f interrupt.o
