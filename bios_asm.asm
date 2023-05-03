BITS 32

[GLOBAL _bits32to16]
[GLOBAL _bits32to16_end]

[GLOBAL _asm_gdt_table]
[GLOBAL _asm_gdt_ptr]
[GLOBAL _asm_idt_ptr]
[GLOBAL _asm_out_reg_ptr]
[GLOBAL _asm_in_reg_ptr]
[GLOBAL _asm_intnum_ptr]

[EXTERN _gdt_ptr]
[EXTERN _gdt_init]
[EXTERN _idt_init]
[EXTERN _real_gdt_ptr]
[EXTERN _real_idt_ptr]

%define REBASE(x)      (0x7C00 + ((x) - _bits32to16))
%define GDTENTRY(x)     ((x) << 3)
%define CODE16          GDTENTRY(6) ;0x30
%define DATA16          GDTENTRY(7) ;0x38
%define CODE32          GDTENTRY(1) ;0x08
%define DATA32          GDTENTRY(2) ;0x10

PG_ON   equ 0x80000000
PG_OFF  equ 0x7FFFFFFF

section .text
BITS 32
_bits32to16:

    pusha
    mov edx, esp
            ; In 32bit protected mode
    cli

    mov ecx, cr0
    and ecx, PG_OFF
    mov cr0, ecx
   
    xor ecx, ecx
    mov ebx, cr3
    mov cr3, ecx

    lgdt [REBASE(_asm_gdt_ptr)]
    lidt [REBASE(_asm_idt_ptr)]
   
    jmp CODE16:REBASE(_protect_16)

_protect_16:use16
             ; In 16bit protected mode
    mov ax, DATA16
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov eax, cr0
    and  al,  ~0x01
    mov cr0, eax

    jmp 0x0:REBASE(_real_16)

_real_16:use16

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov sp, 0x8c00

    sti
    ; ### Save current context ###
    ;pusha
    push ax
    shr eax, 16
    push ax

    push cx
    shr ecx, 16
    push cx

    push dx
    shr edx, 16
    push dx

    push bx
    shr ebx, 16
    push bx

    push bp
    shr ebp, 16
    push bp

    push si
    shr esi, 16
    push si

    push di
    shr edi, 16
    push di

    mov cx, ss
    push cx
    mov cx, gs
    push cx
    mov cx, fs
    push cx
    mov cx, es
    push cx
    mov cx, ds
    push cx
    pushf

    mov ax, sp
    mov edi, _save_16sp
    stosw
    ; ### Load the given context from asm_in_reg_ptr ###
    ; Temporaril change esp to asm_in_reg_ptr
    mov esp, REBASE(_asm_in_reg_ptr)
    popa
    mov sp, 0x9c00
    
    db 0xCD         ; opcode for int
_asm_intnum_ptr:
    ; put the actual interrupt number here
    db 0x00
    ; ### Write current context to asm_out_reg_ptr ###
    mov esp, REBASE(_asm_out_reg_ptr)
    add sp, 28

    pushf
    mov cx, ss
    push cx
    mov cx, gs
    push cx
    mov cx, fs
    push cx
    mov cx, es
    push cx
    mov cx, ds
    push cx
    pusha
    ; ### Restore current context ###
    mov esi, _save_16sp
    lodsw
    mov sp, ax

    popf
    pop cx
    mov ds, cx
    pop cx
    mov es, cx
    pop cx
    mov fs, cx
    pop cx
    mov gs, cx
    pop cx
    mov ss, cx
    ;popa
    pop di
    shl edi, 16
    pop di

    pop si
    shl esi, 16
    pop si

    pop bp
    shl ebp, 16
    pop bp

    pop bx
    shl ebx, 16
    pop bx

    pop dx
    shl edx, 16
    pop dx

    pop cx
    shl ecx, 16
    pop cx

    pop ax
    shl eax, 16
    pop ax

    mov eax, cr0
    inc eax
    mov cr0, eax
    jmp CODE32:REBASE(_protected_32)

_protected_32:use32
    mov ax, DATA32
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov cr3, ebx
    mov ecx, cr0
    or ecx, PG_ON
    mov cr0, ecx
    
    mov esp, edx

    sti
    popa
    ret

    _padding:
        db 0x00
        db 0x00
        db 0x00

    _asm_gdt_table:
        ;resb 64
        dd 0x00
        dd 0x00
        dd 0x00
        dd 0x00
        dd 0x00
        dd 0x00
        dd 0x00
        dd 0x00
        dd 0x00
        dd 0x00
        dd 0x00
        dd 0x00
        dd 0x00
        dd 0x00
        dd 0x00
        dd 0x00

    _asm_gdt_ptr:
        dd 0x00
        dd 0x00

    _asm_idt_ptr:
        dd 0x00
        dd 0x00

    _asm_in_reg_ptr:
        ;resw 14
        dd 0x00
        dd 0x00
        dd 0x00
        dw 0x00

    _asm_out_reg_ptr:
        ;resw 14
        dd 0x00
        dd 0x00
        dw 0x00

    _save_16sp:
        dw 0x0000

_bits32to16_end: