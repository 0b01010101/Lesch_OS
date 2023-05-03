
[GLOBAL _syscall_call]

_syscall_call:
   ; push edi
   ; push esi
    push edx
    push ecx
    push ebx

    mov edx, [esp+16]

    call edx

    add esp, 12
ret