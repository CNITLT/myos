.PHONY:all
all:print.o 

print.o:print.c print.h
	gcc -I../ -m32 -c -o print.o -nostdinc -nostdlib print.c 


.PHONY:clean
clean:
	-rm -f print.o 
