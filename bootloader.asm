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

; Load kernel from disk
mov ah, 0x02
mov al, 32
mov ch, 0
mov cl, 2
mov dh, 0
mov dl, [boot_drive]
mov bx, 0x1000
int 0x13
jc disk_error

; Switch to protected mode
cli
lgdt [gdt_descriptor]
mov eax, cr0
or eax, 0x1
mov cr0, eax
jmp 0x08:init_pm

disk_error:
    jmp $

; GDT
gdt_start:
    dd 0x0, 0x0
gdt_code:
    dw 0xffff, 0x0
    db 0x0, 10011010b, 11001111b, 0x0
gdt_data:
    dw 0xffff, 0x0
    db 0x0, 10010010b, 11001111b, 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[bits 32]
init_pm:
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ebp, 0x90000
    mov esp, ebp
    
    ; Enable FPU properly
    mov eax, cr0
    and eax, 0xFFFFFFF3  ; Clear EM (bit 2) and TS (bit 3)
    or eax, 0x22         ; Set MP (bit 1) and NE (bit 5) for internal FPU errors
    mov cr0, eax
    
    ; Initialize FPU
    fninit
    
    jmp 0x1000

boot_drive db 0

times 510-($-$$) db 0
dw 0xAA55
