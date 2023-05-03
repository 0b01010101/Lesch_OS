BITS 32
DS_USER     equ     0x23
SS_USER     equ     0x23
CS_USER     equ     0x1B
IF_MASK     equ     0x0200

[EXTERN _proc_destroy]          ;in process.c

[GLOBAL _switch_user_mode]
[GLOBAL _switch_kern_mode]
[GLOBAL _copy_code]
;----------------------------------------------------------------------
;Switch to ring 3
;void user_mode_switch(void* entry_point, u32int user_stack_top)
_switch_user_mode:

    mov edx, [esp+4]
    mov eax, DS_USER
    mov ds, ax
    mov es, ax

    mov eax, [esp+8]
    push SS_USER
    push eax
    pushf

    ;pop eax
    ;or eax, IF_MASK
    ;push eax

    push CS_USER
    push edx

    iret 
;-----------------------------------------------------------------
;Switch to ring 0 (used from system call!!!)
_switch_kern_mode:
    jmp _proc_destroy


_copy_code:
    push ebp
    mov ebp, esp

    mov esi, [ebp+8]
    mov edi, [ebp+12]
.loop:
    mov ebx, 0xC3
    cmp [esi], ebx          ;code of command "ret"
    je .out
    mov ebx, 0xCF
    cmp [esi], ebx          ;code of command "iret"
    je .out

    movsb
    jmp .loop
.out:
    movsb

    mov esp, ebp
    pop ebp
    ret