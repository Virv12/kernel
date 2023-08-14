section .multiboot

extern _edata
extern _ebss

align 4
multiboot2_header:
    dd  0xE85250D6
    dd	0
    dd  .size
    dd	-(0xE85250D6 + 0 + .size)

align 8
address_tag:
    dw  2
    dw  0
    dd  .size
    dd  multiboot2_header
    dd  multiboot2_header
    dd  _edata
    dd  _ebss
.size: equ $ - address_tag

align 8
entry_tag:
    dw  3
    dw  0
    dd  .size
    dd  _start
.size: equ $ - entry_tag

align 8
flags_tag:
    dw  4
    dw  0
    dd  .size
    dd  3
.size: equ $ - flags_tag

align 8
terminating_tag:
    dw  0
    dw  0
    dd  8

multiboot2_header.size: equ $ - multiboot2_header

section .text
extern init

global _start
align 16
bits 32
_start:
    ; setup stack
    lea esp, [stack.end]

    ; save multiboot info
    push ebx

    ; check multiboot magic
    cmp eax, 0x36D76289
    jne error

    ; detect cpuid
    pushf
    pop eax
    mov edx, eax
    xor eax, 0x200000
    push eax
    popf
    pushf
    pop eax
    push edx
    popf

    cmp eax, edx
    je error

    ; check extended cpuid
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb error

    ; check long mode support
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz error

    ; enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; enable LME
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; enable TLB
    lea eax, [PDPT]
    or eax, 3
    mov [PML4], eax

    ; identity mmap 1GB huge table
    mov dword [PDPT], 0x83
    mov dword [PDPT + 8], 0x40000083
    mov dword [PDPT + 16], 0x80000083
    mov dword [PDPT + 24], 0xC0000083

    mov eax, PML4
    mov cr3, eax

    ; enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [GDT.hdr]

    ; update segment registers
    mov ax, GDT.data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    pop edi
    jmp GDT.code:init

error:
    hlt

bits 64

section .rodata

; Access bits
PRESENT        equ 1 << 7
NOT_SYS        equ 1 << 4
EXEC           equ 1 << 3
DC             equ 1 << 2
RW             equ 1 << 1
ACCESSED       equ 1 << 0

; Flags bits
GRAN_4K       equ 1 << 7
SZ_32         equ 1 << 6
LONG_MODE     equ 1 << 5

GDT:
.null: equ $ - GDT
    dq 0
.code: equ $ - GDT
    dd 0xFFFF                                   ; Limit & Base (low, bits 0-15)
    db 0                                        ; Base (mid, bits 16-23)
    db PRESENT | NOT_SYS | EXEC | RW            ; Access
    db GRAN_4K | LONG_MODE | 0xF                ; Flags & Limit (high, bits 16-19)
    db 0                                        ; Base (high, bits 24-31)
.data: equ $ - GDT
    dd 0xFFFF                                   ; Limit & Base (low, bits 0-15)
    db 0                                        ; Base (mid, bits 16-23)
    db PRESENT | NOT_SYS | RW                   ; Access
    db GRAN_4K | SZ_32 | 0xF                    ; Flags & Limit (high, bits 16-19)
    db 0                                        ; Base (high, bits 24-31)
.TSS: equ $ - GDT
    dd 0x00000068
    dd 0x00CF8900
.hdr:
    dw $ - GDT - 1
    dq GDT

section .bss

align 16
stack: resb 4096
.end:

align 4096
PML4: resb 4096
PDPT: resb 4096
