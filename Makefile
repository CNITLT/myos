dd=dd of=./c.img bs=512 conv=notrunc
.PHONY:all
all:mbr loader
	$(dd) if=./boot/mbr count=1
	$(dd) if=./boot/loader count=4 seek=2

mbr:
	make -C boot mbr
	

loader:
	make -C boot loader

.PHONY:clean
clean:
	make -C boot clean
