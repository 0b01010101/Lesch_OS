BITS 32
[GLOBAL _gdt_flush]
[GLOBAL _idt_flush]
[GLOBAL _tss_flush]

_gdt_flush:
    mov eax, [esp+4]
    lgdt [eax]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    jmp 0x08:.fl
.fl:
    ret

_idt_flush:
    mov eax, [esp+4]
    lidt [eax]
    ret

_tss_flush:
    mov eax, [esp+4]
    ltr ax
    ret