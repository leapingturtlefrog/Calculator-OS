# Calculator OS - Developer Documentation

## Project Overview

Calculator OS is a minimal, bootable operating system written in x86 assembly and C that boots directly to a simple calculator interface. This is a bare-metal OS with no dependencies on existing operating systems or standard libraries.

## Architecture

### System Components

1. **Bootloader** (`bootloader.asm`) - 512 bytes
2. **Kernel** (`kernel.c`) - Compiled C code
3. **Linker Script** (`linker.ld`) - Memory layout configuration
4. **Build System** (`Makefile`) - Automated build process

### Boot Process

```
BIOS → Bootloader (0x7C00) → Protected Mode → Kernel (0x1000) → Calculator Loop
```

## Features

### Implemented Features

- **Direct BIOS Boot**: Boots from a 1.44MB floppy disk image
- **Protected Mode**: 32-bit protected mode with GDT configuration
- **VGA Text Mode**: Direct video memory manipulation (0xB8000)
- **Keyboard Input**: PS/2 keyboard driver with scancode translation
- **Calculator Operations**: 
  - Addition (+)
  - Subtraction (-)
  - Multiplication (*)
  - Division (/) with zero-division protection
- **Floating Point Support**: Basic float arithmetic and display (4 decimal places)
- **User Interface**:
  - Enter key: Calculate result
  - Backspace: Delete last character
  - ESC: Clear input
  - Shift support for symbols

## Implementation Details

### Bootloader (`bootloader.asm`)

**Purpose**: Load the kernel from disk and transition to 32-bit protected mode

**Key Functions**:

1. **Initialization** (0x7C00)
   - Sets up segment registers (DS, ES, SS)
   - Saves boot drive number
   - Initializes stack at 0x7C00

2. **Disk Loading**
   - Uses BIOS interrupt 0x13 (AH=0x02)
   - Reads 32 sectors starting from sector 2
   - Loads kernel to physical address 0x1000
   - Error handling with retry mechanism

3. **Protected Mode Transition**
   - Defines GDT with null, code, and data descriptors
   - Code segment: Base=0, Limit=0xFFFFF, Type=Execute/Read
   - Data segment: Base=0, Limit=0xFFFFF, Type=Read/Write
   - Sets CR0 PE bit to enable protected mode
   - Far jump to flush pipeline and load CS

4. **GDT Structure**
   ```
   Offset 0x00: Null descriptor
   Offset 0x08: Code segment (10011010b access, 11001111b granularity)
   Offset 0x10: Data segment (10010010b access, 11001111b granularity)
   ```

**Boot Signature**: 0xAA55 at bytes 510-511

### Kernel (`kernel.c`)

**Entry Point**: `kernel_main()` with `section(".text.start")` attribute

#### Memory Layout

- **VGA Memory**: 0xB8000 (80x25 text mode, 2 bytes per character)
- **Kernel Load Address**: 0x1000 (defined in linker.ld)
- **Stack**: 0x90000 (set by bootloader)

#### Core Components

1. **VGA Driver**
   - Direct memory-mapped I/O to 0xB8000
   - Character format: [byte 0: ASCII] [byte 1: color attribute]
   - Color format: High nibble = background, low nibble = foreground
   - Hardware cursor control via ports 0x3D4/0x3D5

2. **Keyboard Driver**
   - Polls keyboard status port (0x64)
   - Reads scancodes from data port (0x60)
   - Scancode-to-ASCII translation tables (normal & shift)
   - Shift state tracking (both left and right shift keys)
   - Filters key release events (bit 7 set)

3. **Calculator Engine**
   - **Input Buffer**: 256 bytes max
   - **Parser**: Extracts operands and operators
   - **Number Parsing**: Supports integers and decimals
   - **Operations**: Single operator between two numbers
   - **Format**: `<number1><operator><number2>`

4. **Floating Point Math**
   - No FPU libraries; uses GCC built-in float operations
   - Display precision: 4 decimal places
   - Integer extraction: Cast to int
   - Fractional extraction: Multiply and truncate method

#### Key Functions

**Hardware I/O**:
```c
unsigned char inb(unsigned short port)   // Read from I/O port
void outb(unsigned short port, unsigned char value)  // Write to I/O port
```

**Display**:
```c
void clear_screen()                      // Clear VGA buffer
void print_char(char c, color, x, y)    // Print at position
void print_string(const char* str, color) // Print at cursor
void print_number(int num)               // Print integer
void print_float(float num)              // Print float (4 decimals)
void update_cursor()                     // Sync hardware cursor
```

**Input**:
```c
char get_key()                           // Wait for keypress, return ASCII
```

**Calculator**:
```c
float calculate()                        // Parse input and compute
float parse_number()                     // Extract number from buffer
```

### Linker Script (`linker.ld`)

**Purpose**: Define memory layout for the kernel binary

**Configuration**:
- Entry point: `kernel_main`
- Base address: 0x1000 (matches bootloader load address)
- Section alignment: 4 bytes
- Section order: `.text.start`, `.text`, `.rodata`, `.data`, `.bss`
- Discarded sections: `.comment`, `.eh_frame`, `.note.gnu.build-id`

**Critical**: `.text.start` must be first to ensure `kernel_main` is at 0x1000

### Build System (`Makefile`)

**Tools Required**:
- `nasm`: Assembler for bootloader
- `gcc`: C compiler (with 32-bit support)
- `ld`: GNU linker
- `objcopy`: Binary conversion tool
- `dd`: Disk image creation
- `qemu-system-i386`: Emulator for testing

**Compiler Flags**:
```makefile
CFLAGS = -m32              # 32-bit mode
         -ffreestanding    # No hosted environment
         -nostdlib         # No standard library
         -fno-builtin      # No built-in functions
         -fno-stack-protector  # No stack canary
         -nostartfiles     # No startup files
         -nodefaultlibs    # No default libraries
         -Wall -Wextra     # All warnings
         -O0               # No optimization (for debugging)
         -c                # Compile only
```

**Linker Flags**:
```makefile
LDFLAGS = -m elf_i386      # 32-bit ELF
          -T linker.ld     # Use custom linker script
          -nostdlib        # No standard library
```

**Build Process**:
1. Assemble bootloader to raw binary (512 bytes)
2. Compile kernel.c to object file
3. Link object file to ELF executable
4. Convert ELF to raw binary with `objcopy`
5. Concatenate bootloader + kernel
6. Pad to 1.44MB (2880 sectors × 512 bytes)

**Make Targets**:
- `make` or `make all`: Build complete OS image
- `make run`: Run existing OS image in QEMU (no rebuild)
- `make run-new`: Build and run in QEMU
- `make clean`: Remove all build artifacts
- `make time-compile`: Benchmark compilation time using hyperfine
- `make time-boot`: Benchmark boot time using hyperfine
- `make benchmark`: Run both compilation and boot time benchmarks

## Development Guidelines

### Adding New Features

1. **New Calculator Operations**:
   - Add case to switch statement in `calculate()`
   - Ensure operator is in keyboard scancode table

2. **Enhanced Display**:
   - Modify VGA constants for colors
   - Add to `print_*` functions as needed
   - Update cursor management for new layouts

3. **Keyboard Enhancements**:
   - Extend scancode tables in `get_key()`
   - Handle special keys (Ctrl, Alt) by tracking state

4. **Multi-operation Calculator**:
   - Implement operator precedence parser
   - Add parentheses support
   - Maintain operator stack

### Memory Constraints

**Bootloader**: Exactly 512 bytes (enforced by BIOS)
- Current usage: ~480 bytes
- Available: ~30 bytes for additions

**Kernel**: Limited by disk sectors loaded
- Current: 32 sectors = 16 KB
- Increase by modifying `mov al, 32` in bootloader.asm

**Stack**: 0x90000 - 0x1000 = ~572 KB available

**VGA Memory**: 80×25×2 = 4000 bytes (fixed by hardware)

### Debugging Tips

1. **Bootloader Issues**:
   - Check boot signature (0xAA55)
   - Verify disk read with error messages
   - Use QEMU monitor: `Ctrl-Alt-2`

2. **Kernel Loading**:
   - Verify sector count is sufficient
   - Check linker.ld entry point
   - Ensure `.text.start` section is first

3. **Protected Mode**:
   - GDT must be properly aligned
   - Segment selectors must match GDT offsets
   - Far jump is required after setting CR0

4. **Keyboard Not Working**:
   - Verify scancode tables
   - Check port addresses (0x60, 0x64)
   - Ensure bit masking for key releases

5. **Display Issues**:
   - Confirm VGA_MEMORY = 0xB8000
   - Check color byte format
   - Verify cursor position calculations

### Testing

**Unit Testing**: Not feasible (no OS, no standard library)

**Integration Testing**:
```bash
make run  # Launch in QEMU
# Test cases:
# - Simple addition: 5+3 (expect 8.0)
# - Subtraction: 10-2 (expect 8.0)
# - Multiplication: 6*7 (expect 42.0)
# - Division: 8/2 (expect 4.0)
# - Division by zero: 5/0 (expect 0.0)
# - Decimals: 5.5+2.5 (expect 8.0)
# - ESC clears input
# - Backspace deletes last char
```

**Hardware Testing**:
- Write os.img to USB drive
- Boot on real x86 hardware
- Note: Some modern hardware may not support legacy BIOS boot

## Common Issues

### Compilation Errors

**Error**: `undefined reference to '__stack_chk_fail'`
- **Solution**: Add `-fno-stack-protector` to CFLAGS

**Error**: `cannot find -lgcc`
- **Solution**: Add `-nostdlib` and `-nodefaultlibs` to LDFLAGS

**Error**: `kernel_main not at expected address`
- **Solution**: Ensure `.text.start` section is first in linker.ld

### Runtime Issues

**Symptom**: Bootloader loads but kernel doesn't start
- **Check**: Verify 32 sectors are loaded (or increase count)
- **Check**: Confirm kernel_main has `section(".text.start")`

**Symptom**: Keyboard input not working
- **Check**: QEMU needs focus (click on window)
- **Check**: Scancode tables include the key

**Symptom**: Display corruption
- **Check**: Cursor position calculations
- **Check**: Screen boundary checks

## Performance Characteristics

- **Boot Time**: ~200-400ms (QEMU)
- **Input Latency**: <100ms (polling-based)
- **Calculation Speed**: Instant (no complex operations)
- **Memory Footprint**: 
  - Bootloader: 512 bytes
  - Kernel: ~5-10 KB (depending on optimization)
  - Runtime: ~256 bytes (input buffer + globals)

## Security Considerations

**Current Security**: None (it's a calculator OS!)

**Potential Issues**:
- Buffer overflow in input_buffer (limited to 255 chars)
- No input validation beyond buffer size
- Division by zero returns 0.0 (silent failure)

**Not Applicable**:
- User permissions (no users)
- File access (no filesystem)
- Network security (no network)

## License & Credits

This is an educational project demonstrating:
- x86 assembly programming
- OS bootloader development
- Protected mode initialization
- Bare-metal C programming
- Direct hardware access

**Learning Resources**:
- OSDev Wiki: https://wiki.osdev.org/
- Intel x86 Manual: Software Developer's Manual Vol. 3
- BIOS Interrupt List: http://www.ctyme.com/intr/int.htm

## Quick Reference

### Memory Map
```
0x0000 - 0x03FF: Real Mode IVT (not used after protected mode)
0x0400 - 0x04FF: BIOS Data Area
0x0500 - 0x7BFF: Free (not used)
0x7C00 - 0x7DFF: Bootloader (512 bytes)
0x7E00 - 0x0FFF: Free
0x1000 - 0x????:  Kernel code/data
0xA0000 - 0xBFFFF: Video memory
0xB8000 - 0xB8FA0: VGA text mode (4000 bytes)
0x90000: Stack pointer (grows downward)
```

### Important Constants
```c
VGA_MEMORY    = 0xB8000
VGA_WIDTH     = 80
VGA_HEIGHT    = 25
KEYBOARD_PORT = 0x60
KEYBOARD_STATUS = 0x64
KERNEL_OFFSET = 0x1000
```

### Build Commands
```bash
make              # Build os.img
make run          # Run existing os.img in QEMU (no rebuild)
make run-new      # Build and run in QEMU
make clean        # Remove build artifacts
make time-compile # Benchmark compilation time
make time-boot    # Benchmark boot time
make benchmark    # Run all benchmarks

# Manual build:
nasm -f bin bootloader.asm -o bootloader.bin
gcc -m32 -ffreestanding -c kernel.c -o kernel.o
ld -m elf_i386 -T linker.ld -o kernel.elf kernel.o
objcopy -O binary kernel.elf kernel.bin
cat bootloader.bin kernel.bin > os.img
dd if=/dev/zero bs=512 count=2880 >> os.img
dd if=os.img of=os.img bs=512 count=2880 conv=notrunc
```

### QEMU Options
```bash
# Basic run
qemu-system-i386 -drive file=os.img,format=raw,if=floppy

# With debugging
qemu-system-i386 -drive file=os.img,format=raw,if=floppy -s -S
# Then: gdb, target remote :1234

# With monitor
qemu-system-i386 -drive file=os.img,format=raw,if=floppy -monitor stdio
```

---

**Last Updated**: 2025
**Version**: 0.1
**Status**: Educational/Experimental
