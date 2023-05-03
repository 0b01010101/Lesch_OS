BITS 32
[EXTERN _curr_thread]
[EXTERN _curr_proc]
[EXTERN _multi_task]
[EXTERN _tss]
[EXTERN _wprint]
[EXTERN _curr_tty16]

[GLOBAL _switch_cr3]
[GLOBAL _switch_task]
[GLOBAL _switch_the_task]
[GLOBAL _read_eflags]
[GLOBAL _start]
[GLOBAL _stop]

;_multi_task:    db 0x00
_str: db 'proc(%x)\n', 0
_str2: db 'pd(%x)\n', 0

_switch_task:
    push ebp
    pushf
    cli

    mov edx, [_curr_thread]
    mov [edx+32], esp

    mov ecx, [edx+4]
    mov [_curr_thread], ecx

    mov edx, [_curr_thread]

    mov ebx, [edx+16]      ;current_thread->process --> EBX
    mov [_curr_proc], ebx
    mov ecx, [ebx+16]      ;process->page_dir --> ECX
    mov cr3, ecx           ;reload CR3

    mov ecx, [ebx+32]      ;process->tty --> ECX
    mov [_curr_tty16], ecx  

    mov esp, [edx+32]
    
;Модифицируем вершину стека ядра в TSS
    mov eax, [edx+44]       ;Читаем вершину стека из структуры потока
    mov edx, _tss            ;Грузим EDX адресом TSS        
    mov [edx+4], eax        ;Пишем вершину стека в поле tss.esp0

    popf
    pop ebp
    ret

;Start scheduler
_start:

    mov eax, [_multi_task]
    dec eax
    ;mov eax, 0x01
    xchg [_multi_task], eax
    ret

;Stop scheduler
_stop:
    mov eax, [_multi_task]
    inc eax
    ;xor eax, eax
    xchg [_multi_task], eax
    ret

;physaddr *switch_cr3(physaddr *paddr_need);
_switch_cr3:
    mov ebx, [esp+4]
    mov eax, cr3
    mov cr3, ebx
    ret 

_read_eflags:
    pushf   
    pop eax
    ret


_switch_the_task:               
    push ebp
    pushf
    cli

    mov edx, [_curr_thread]
    mov [edx+32], esp

    mov ecx, [esp+12]
    mov [_curr_thread], ecx

    mov edx, [_curr_thread]

    mov ebx, [edx+16]       ;current_thread->process --> EBX
    mov ecx, [ebx+16]       ;process->page_dir --> ECX
    mov cr3, ecx            ;reload CR3

    mov esp, [edx+32]

    mov eax, [edx+44]
    mov edx, _tss
    mov [edx+4], eax

    popf
    pop ebp
    ret
