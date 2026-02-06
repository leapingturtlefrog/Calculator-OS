CC = gcc
AS = nasm
LD = ld
CFLAGS = -m32 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -mno-sse -mno-sse2 -mfpmath=387 -O2 -c
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

OBJECTS = kernel.o math.o extras.o

all: os.img web/os.img

bootloader.bin: bootloader.asm
	$(AS) -f bin bootloader.asm -o bootloader.bin

kernel.o: kernel.c math.h extras.h
	$(CC) $(CFLAGS) kernel.c -o kernel.o

math.o: math.c math.h
	$(CC) $(CFLAGS) math.c -o math.o

extras.o: extras.c extras.h
	$(CC) $(CFLAGS) extras.c -o extras.o

kernel.bin: $(OBJECTS)
	$(LD) $(LDFLAGS) -o kernel.elf $(OBJECTS)
	objcopy -O binary kernel.elf kernel.bin

os.img: bootloader.bin kernel.bin
	cat bootloader.bin kernel.bin > os.img
	dd if=/dev/zero bs=512 count=2880 >> os.img 2>/dev/null
	dd if=os.img of=os.img bs=512 count=2880 conv=notrunc 2>/dev/null

web/os.img: os.img
	mkdir -p web
	cp os.img web/os.img

run: os.img
	qemu-system-i386 -drive file=os.img,format=raw,if=floppy

# Test mode - runs headlessly with serial output to terminal
test: os.img
	qemu-system-i386 -drive file=os.img,format=raw,if=floppy -serial stdio -display curses

# Debug mode - no display, just serial output (runs for 5 seconds)
test-headless: os.img
	timeout 5 qemu-system-i386 -drive file=os.img,format=raw,if=floppy -serial stdio -display none || true

clean:
	rm -f *.o *.bin *.elf os.img web/os.img

.PHONY: all run clean test test-headless
