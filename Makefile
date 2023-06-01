dd=dd of=./c.img bs=512 conv=notrunc
.PHONY:all
all:mbr loader kernel.bin timer.o
	$(dd) if=./boot/mbr count=1
	$(dd) if=./boot/loader count=4 seek=2
	$(dd) if=./kernel/kernel.bin count=200 seek=9
mbr:
	make -C boot mbr
	
loader:
	make -C boot loader

kernel.bin:
	make -C kernel kernel.bin

timer.o:
	make -C device timer.o

.PHONY:clean
clean:
	make -C boot clean
	make -C kernel clean
	make -C device clean