CC = gcc
AS = nasm
LD = ld
CFLAGS = -m32 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -mno-sse -mno-sse2 -mfpmath=387 -O2 -c
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

OUT = out
OBJDIR = $(OUT)/obj
BINDIR = $(OUT)/bin
OSDIR = $(OUT)/os

OBJECTS = $(OBJDIR)/kernel.o $(OBJDIR)/math.o $(OBJDIR)/extras.o

all: $(OSDIR)/os.img web/os.img

$(BINDIR)/bootloader.bin: bootloader.asm
	mkdir -p $(BINDIR)
	$(AS) -f bin bootloader.asm -o $(BINDIR)/bootloader.bin

$(OBJDIR)/kernel.o: kernel.c math.h extras.h
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) kernel.c -o $(OBJDIR)/kernel.o

$(OBJDIR)/math.o: math.c math.h
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) math.c -o $(OBJDIR)/math.o

$(OBJDIR)/extras.o: extras.c extras.h
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) extras.c -o $(OBJDIR)/extras.o

$(BINDIR)/kernel.bin: $(OBJECTS)
	mkdir -p $(BINDIR)
	$(LD) $(LDFLAGS) -o $(BINDIR)/kernel.elf $(OBJECTS)
	objcopy -O binary $(BINDIR)/kernel.elf $(BINDIR)/kernel.bin

$(OSDIR)/os.img: $(BINDIR)/bootloader.bin $(BINDIR)/kernel.bin
	mkdir -p $(OSDIR)
	cat $(BINDIR)/bootloader.bin $(BINDIR)/kernel.bin > $(OSDIR)/os.img
	dd if=/dev/zero bs=512 count=2880 >> $(OSDIR)/os.img 2>/dev/null
	dd if=$(OSDIR)/os.img of=$(OSDIR)/os.img bs=512 count=2880 conv=notrunc 2>/dev/null

web/os.img: $(OSDIR)/os.img
	mkdir -p web
	cp $(OSDIR)/os.img web/os.img

run: $(OSDIR)/os.img
	qemu-system-i386 -drive file=$(OSDIR)/os.img,format=raw,if=floppy

# Test mode - runs headlessly with serial output to terminal
test: $(OSDIR)/os.img
	qemu-system-i386 -drive file=$(OSDIR)/os.img,format=raw,if=floppy -serial stdio -display curses

# Debug mode - no display, just serial output (runs for 5 seconds)
test-headless: $(OSDIR)/os.img
	timeout 5 qemu-system-i386 -drive file=$(OSDIR)/os.img,format=raw,if=floppy -serial stdio -display none || true

clean:
	rm -rf $(OUT)

.PHONY: all run clean test test-headless
