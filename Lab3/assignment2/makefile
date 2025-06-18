LD=ld
run:
	@qemu-system-i386 -hda hd.img -serial null -parallel stdio 
debug:
	@qemu-system-i386 -s -S -hda hd.img -serial null -parallel stdio &
	@sleep 1
	@gnome-terminal -- gdb -q -x gdbinit

build:
	@nasm -g -f bin mbr.asm -o mbr.bin
	@dd if=mbr.bin of=hd.img bs=512 count=1 seek=0 conv=notrunc
	@nasm -g -f bin bootloader.asm -o bootloader.bin
	@dd if=bootloader.bin of=hd.img bs=512 count=5 seek=1 conv=notrunc

symbol:
	@nasm -g -f elf32 mbr.asm -o mbr.o
	@${LD} -o mbr.symbol -melf_i386 -N mbr.o -Ttext 0x7c00

	@nasm -g -f elf32 bootloader.asm -o bootloader.o
	@${LD} -o bootloader.symbol -melf_i386 -N bootloader.o -Ttext 0x7e00

clean:
	@rm -fr *.bin *.o *.symbol
