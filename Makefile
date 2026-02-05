CC = gcc
AS = nasm
LD = ld
CFLAGS = -m32 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -O0 -c
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

all: os.img

bootloader.bin: bootloader.asm
	$(AS) -f bin bootloader.asm -o bootloader.bin

kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

kernel.bin: kernel.o
	$(LD) $(LDFLAGS) -o kernel.elf kernel.o
	objcopy -O binary kernel.elf kernel.bin

os.img: bootloader.bin kernel.bin
	cat bootloader.bin kernel.bin > os.img
	dd if=/dev/zero bs=512 count=2880 >> os.img 2>/dev/null
	dd if=os.img of=os.img bs=512 count=2880 conv=notrunc 2>/dev/null

run: os.img
	qemu-system-i386 -drive file=os.img,format=raw,if=floppy

clean:
	rm -f *.o *.bin *.elf os.img

.PHONY: all run clean
