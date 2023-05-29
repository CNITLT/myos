NASMFLAGS = -Iinclude

mbr:mbr.asm
	nasm $(NASMFLAGS) mbr.asm 

loader:loader.asm
	nasm $(NASMFLAGS) loader.asm 

.PHONY:clean
clean:
	-rm -f mbr
	-rm -f loader