BITS 32
KERN_OFFSET     equ 0x200000
[EXTERN _kernel]
[GLOBAL _go]


_go:
    mov esp, KERN_OFFSET -4
    cli
    push esp;
    push ebx

    call _kernel            ;main.c
    
    hlt
loop_k:
    jmp loop_k
