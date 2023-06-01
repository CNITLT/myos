CC=gcc
CFLAGS=-m32 -I../lib/kernel -I../lib -nostdinc -nostdlib
LD32=-m elf_i386
.PHONY:all
all:timer.o

timer.o:timer.h timer.c
	$(CC) $(CFLAGS) timer.c -c -o timer.o

.PHONY:clean
clean:
	-rm -f timer.o