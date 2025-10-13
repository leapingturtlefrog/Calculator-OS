ENTRY(kernel_main)

SECTIONS {
    /* Kernel will be loaded at 0x1000 physical address */
    . = 0x1000;

    .text ALIGN(4) : {
        /* Ensure entry point is at the beginning */
        *(.text.start)
        *(.text)
    }

    .rodata ALIGN(4) : {
        *(.rodata)
    }

    .data ALIGN(4) : {
        *(.data)
    }

    .bss ALIGN(4) : {
        *(COMMON)
        *(.bss)
    }

    /* Discard unnecessary sections */
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
} 