[bits 16]
[org 0x7c00]

; Initialize segment registers
mov ax, 0
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7c00

; Save boot drive
mov [boot_drive], dl

; Print welcome message
mov si, welcome_msg
call print_string

; Load kernel from disk (second sector)
mov si, loading_msg
call print_string

mov ah, 0x02            ; BIOS read sector function
mov al, 32              ; Number of sectors to read (increased for bigger kernel)
mov ch, 0               ; Cylinder number
mov cl, 2               ; Sector number (starts from 1, boot sector is 1)
mov dh, 0               ; Head number
mov dl, [boot_drive]    ; Drive number
mov bx, KERNEL_OFFSET   ; Load kernel to address 0x1000
int 0x13                ; Call BIOS interrupt
jc disk_error           ; Jump if disk read error (carry flag set)

; Print success message
mov si, success_msg
call print_string

; Print "Switching to protected mode" message
mov si, pm_msg
call print_string

; Switch to 32-bit protected mode
call switch_to_pm       ; This will never return

; Error handler
disk_error:
    mov si, disk_error_msg
    call print_string
    jmp $

; Function to print a null-terminated string
print_string:
    pusha
    mov ah, 0x0E        ; BIOS teletype function

.loop:
    lodsb               ; Load next character from SI into AL
    test al, al         ; Check if character is null
    jz .done            ; If null, we're done
    int 0x10            ; Call BIOS interrupt to print character
    jmp .loop           ; Repeat for next character

.done:
    popa
    ret

; GDT for 32-bit protected mode
gdt_start:
    ; Null descriptor (required)
    dd 0x0              ; 4 bytes of zeros
    dd 0x0              ; 4 bytes of zeros

gdt_code:               ; Code segment descriptor
    dw 0xffff           ; Limit (bits 0-15)
    dw 0x0              ; Base (bits 0-15)
    db 0x0              ; Base (bits 16-23)
    db 10011010b        ; Access byte: Present=1, DPL=00, S=1, Type=1010 (code, read/execute)
    db 11001111b        ; Granularity byte: G=1, D/B=1, L=0, AVL=0, Limit(bits 16-19)=1111
    db 0x0              ; Base (bits 24-31)

gdt_data:               ; Data segment descriptor
    dw 0xffff           ; Limit (bits 0-15)
    dw 0x0              ; Base (bits 0-15)
    db 0x0              ; Base (bits 16-23)
    db 10010010b        ; Access byte: Present=1, DPL=00, S=1, Type=0010 (data, read/write)
    db 11001111b        ; Granularity byte: G=1, D/B=1, L=0, AVL=0, Limit(bits 16-19)=1111
    db 0x0              ; Base (bits 24-31)

gdt_end:                ; Used to calculate GDT size

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Size of the GDT (always one less than true size)
    dd gdt_start                ; Start address of the GDT

; Define segments selectors (offsets into the GDT)
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; Function to switch to protected mode
switch_to_pm:
    cli                     ; Disable interrupts
    
    ; Load GDT
    lgdt [gdt_descriptor]
    
    ; Set PE (Protection Enable) bit in CR0
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    
    ; Far jump to 32-bit code
    jmp CODE_SEG:init_pm

[bits 32]
; Initialize 32-bit protected mode
init_pm:
    ; Initialize segment registers for protected mode
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Set up stack
    mov ebp, 0x90000
    mov esp, ebp
    
    ; Print a message directly to video memory to indicate PM is working
    mov ebx, PM_MSG_ADDR
    mov [ebx], DWORD 0x4B4F4F4C   ; "LOOK" in ASCII with color
    
    ; Jump to kernel
    jmp KERNEL_OFFSET

; Data section
welcome_msg db 'Loading Calculator OS...', 13, 10, 0
loading_msg db 'Reading kernel from disk...', 13, 10, 0
success_msg db 'Successfully loaded kernel', 13, 10, 0
pm_msg db 'Switching to protected mode...', 13, 10, 0
disk_error_msg db 'Disk Error!', 13, 10, 0
boot_drive db 0

; Constants
KERNEL_OFFSET equ 0x1000     ; Memory offset where kernel will be loaded
PM_MSG_ADDR equ 0xB8000      ; Video memory address

; Padding and boot signature
times 510-($-$$) db 0   ; Pad to 510 bytes
dw 0xAA55               ; Boot signature 