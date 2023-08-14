OUTPUT_FORMAT(binary)
/* OUTPUT_FORMAT(elf64-x86-64) */
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    . = 0x100000;

    .multiboot : { *(.multiboot*) }
    .text      : { *(.text*) }

    .rodata    : { *(.rodata*) }
    .data      : { *(.data*) _edata = .;}
    .bss       : { *(.bss*)  _ebss = .;}

    /DISCARD/  : { *(*) }
}
