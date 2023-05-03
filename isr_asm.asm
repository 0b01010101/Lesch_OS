BITS 32
[EXTERN _isr_handler]        ;in isr.c
[EXTERN _irq_handler]        ;in isr.c

[GLOBAL _isr48]

%macro ISR_NOERRCODE 1
    [GLOBAL _isr%1]
    _isr%1:
        cli
        push byte 0
        push byte %1
        jmp isr_common
%endmacro
    
%macro ISR_ERRCODE 1
    [GLOBAL _isr%1]
    _isr%1:
        cli
        push byte %1
        jmp isr_common
%endmacro

%macro IRQ 2
    [GLOBAL _irq%1]
    _irq%1:
        cli
        push byte 0;
        push byte %2
        jmp irq_common
%endmacro

irq_common:
    pusha
    mov ax, ds
    push eax

    mov ax, 0x10;
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax

    call _irq_handler

    pop eax
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    popa

    add esp, 8
    sti
iret

isr_common:
    pusha
    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax

    call _isr_handler

    pop eax
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    popa

    add esp, 8
    sti
iret

_isr48: 
    push 0
    push 48

    pusha
    push ds

    mov ax, 0x10
    mov ds, ax

    call _isr_handler

    pop ds
    pop edi
    pop esi
    pop ebp
    add esp, 4              ;should be "pop eax" but eax is saved;
    pop ebx
    pop edx
    pop ecx

    add esp, 12
iret

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE 8
ISR_NOERRCODE 9

ISR_ERRCODE 10
ISR_ERRCODE 11
ISR_ERRCODE 12
ISR_ERRCODE 13
ISR_ERRCODE 14

ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

IRQ 0,  32
IRQ 1,  33
IRQ 2,  34
IRQ 3,  35
IRQ 4,  36
IRQ 5,  37
IRQ 6,  38
IRQ 7,  39
IRQ 8,  40
IRQ 9,  41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

