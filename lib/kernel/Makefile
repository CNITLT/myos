.PHONY:all
all:print.o print_asm.o

print.o:print_asm.o print.c print.h
	gcc -I../ -m32 -c -o print_temp.o -nostdinc -nostdlib print.c 
	ld -r -m elf_i386 print_temp.o print_asm.o -o print.o
	rm -f print_temp.o
print_asm.o:print.asm
	nasm print.asm -o print_asm.o -f elf


.PHONY:clean
clean:
	-rm -f print.o 
	-rm -f print_asm.o 