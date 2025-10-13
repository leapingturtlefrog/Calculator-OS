CC = gcc
AS = nasm
LD = ld
CFLAGS = -m32 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -O0 -c
ASFLAGS = -f bin
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

all: os.img

# Assemble bootloader
bootloader.bin: bootloader.asm
	$(AS) $(ASFLAGS) bootloader.asm -o bootloader.bin

# Compile kernel
kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

# Link kernel
kernel.bin: kernel.o
	$(LD) $(LDFLAGS) -o kernel.elf kernel.o
	objcopy -O binary kernel.elf kernel.bin

# Create disk image
os.img: bootloader.bin kernel.bin
	cat bootloader.bin kernel.bin > os.img
	# Pad to make it a valid floppy image (1.44MB)
	dd if=/dev/zero bs=512 count=2880 >> os.img
	dd if=os.img of=os.img bs=512 count=2880 conv=notrunc

# Run in QEMU
run-new: os.img
	qemu-system-i386 -drive file=os.img,format=raw,if=floppy

run:
	qemu-system-i386 -drive file=os.img,format=raw,if=floppy

clean:
	rm -f *.o *.bin *.elf os.img

time-compile:
	@echo "Compilation Time"
	@hyperfine --warmup 1 --runs 10 --prepare 'make clean > /dev/null 2>&1' 'make all > /dev/null 2>&1'

time-boot: os.img
	@echo "Boot Time"
	@hyperfine --warmup 1 --runs 10 './measure_boot_time.sh'

benchmark: time-compile time-boot

.PHONY: all run clean time-compile time-boot benchmark 