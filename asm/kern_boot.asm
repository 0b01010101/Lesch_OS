;==========================================================================
BITS 16  
KERN_OFFSET         equ 0x200000    ;2mb
KERN_SIZE           equ 0x1000      ;size of dword (65536 bytes)
MEMORY_MAP_SIZE     equ 0x01F4      ;500 bytes
FLOP_BUFF_SIZE      equ 0x5000      ;512(bytes) in 1 sector of floppy; we use 40 sectors => 512*40 == 20480(bytes)
FLOP_SECTORS        equ 54
FLOP_BYTES_SECTOR   equ 512
FLOP_MAX_SECTORS    equ 2846

kernel_buffer:        ;Kernel in real mode
    ; CS, DS, == ES now set to 0000h, SS to 9000h 
    push real_kern_ok
    call Kern_printf

    mov eax, 0FCDBEFABh
    shr eax, 16
    cmp ax, 0FCDBh
    jne fail_real_mode

    push memory_map
    call init_memory_map

    ; Open gate A20 through the System Control Port A
    in al, 0x92
    or al, 0x02
    out 0x92, al

    ; Disable maskable interrupts
    cli    
    ; Disable non-maskable interrupts
    in al, 0x70
    or al, 0x80
    out 0x70, al

    lgdt [gdt_str]

    ; Switch to protected mode
    mov eax, cr0
    or al, 0x01
    mov cr0, eax

    jmp dword 0x08:Kern_32
    
    finish_kern_real:
        hlt
        jmp finish_kern_real

    fail_real_mode:
        mov si, kern_real_fail
        call Kern_printf
        jmp finish_kern_real

    init_memory_map:
        push bp
        mov bp, sp

        push eax
        push ebx
        push ecx
        push edx
        push di

        mov dword [mmap_length], 0

        xor ebx, ebx
        xor ecx, ecx
        xor di,  di
         
        mov edx, 0x534D4150     ;SMAP
        mov di,  [bp+4]         ;es:di - addr of buf memory_map
        add di, 4;

        .loop:
            mov ecx, 24         
            mov eax, 0x0000E820
            mov [di+20], dword 1;
            int 15h             ;output eax, ebx, ecx
            jc .failed
            
            mov edx, 0x534D4150
            cmp eax, edx
            jne .failed

        .check:
            jcxz .skip
            cmp cx, 20                      ; got a 24 byte ACPI 3.X response?
            jbe .nodat
            test byte [di+20], 1            ; if so: is the "ignore this data" bit clear?
            je .skip

        .nodat:
            mov [di-4], ecx;
            add ecx, 4;
            add dword [mmap_length], ecx
            mov ecx, [di+8]                 ; get lower uint32_t of memory region length
            or  ecx, [di+12]                ; "or" it with upper uint32_t to test for zero
            jz .skip                        ; if length uint64_t is 0, skip entry

            add di, 24

        .skip:
            cmp ebx, 0
            jne .loop

        .end:
            mov di, mmap_addr
            mov si, memory_map
            mov [di], si

            ;mov ecx, [mmap_length]

        pop di
        pop edx
        pop ecx
        pop ebx
        pop eax

        mov sp, bp
        pop bp
        clc
    ret 2
        .failed:
            pop di
            pop edx
            pop ecx
            pop ebx
            pop eax

        mov sp, bp
        pop bp
        stc
    ret 2

    Kern_printf:
            push bp
            mov bp, sp

            push ax
            push bx 
            push si

            mov si, [bp+4]
            cld 
            mov ah, 0x0E        ; (int 0x10) BIOS function index (write a charachter to the active video page)
            mov bh, 0x00        ; (int 0x10) video page number

            .loop:
                lodsb
                test al, al
                jz .pr_exit 
                int 10h
                jmp .loop
            .pr_exit:

            pop si
            pop bx
            pop ax

            mov sp, bp
            pop bp
    ret 2
;----------------------------  data ------------------------------------
    kern_real_fail db 'Fail in real mode Kernel!!!', 0x0d, 0x0a, 0
    real_kern_ok db 'In real mode Kernel successfully!', 0x0d, 0x0a, 0

    floppy_buffer_add:
        times (FLOP_SECTORS * FLOP_BYTES_SECTOR) - ($ - kernel_buffer) db 0

    mboot_struct:
       flags:               dd 00
       mem_lower:           dd 00
       mem_upper:           dd 00
       boot_device:         dd 00
       cmdline:             dd 00
       mods_count:          dd 00
       mods_addr:           dd 00
       num:                 dd 00
       size:                dd 00
       addr:                dd 00
       shndx:               dd 00
       mmap_length:         dd 00
       mmap_addr:           dd 00
       drives_length:       dd 00
       drives_addr:         dd 00
       config_table:        dd 00
       boot_loader_name:    dd 00
       apm_table:           dd 00
       vbe_control_info:    dd 00
       vbe_mode_info:       dd 00
       vbe_mode:            dd 00
       vbe_interface_seg:   dd 00
       vbe_interface_off:   dd 00
       vbe_interface_len:   dd 00
    
    memory_map:
        times MEMORY_MAP_SIZE db 0
;-----------------------------------------------------------------------
;============================
;Kernel (protect mode)
BITS 32

align 8
gdt:
;   null descriptor
                            dd 0
                            dd 0
; code descriptor 32 bit
    p_gdt_code_limit_low:      dw 0FFFFh
    p_gdt_code_base_low:       dw 0
    p_gdt_code_base_middle:    db 0
    p_gdt_code_access:         db 10011010b         ;exec-read; DPL=0; Present
    p_gdt_code_granularity:    db 11001111b         ;length=1111b; AVAL=0; D=1; G=1
    p_gdt_code_base_high:      db 0

; data descriptor 32 bit
    p_gdt_data_limit_low:      dw 0FFFFh
    p_gdt_data_base_low:       dw 0
    p_gdt_data_base_middle:    db 0
    p_gdt_data_access:         db 10010010b         ;read/write; DPL=0; Present
    p_gdt_data_granularity:    db 11001111b         ;length=1111b; AVAL=0; D=1; G=1
    p_gdt_data_base_high:      db 0

;code descriptor 16 bit
    r_gdt_code_limit_low:       dw 0FFFFh
    r_gdt_code_base_low:        dw 0
    r_gdt_code_base_middle:     db 0
    r_gdt_code_access:          db 10011010b            ;exec-read; DPL=0; Present
    r_gdt_code_granularity:     db 00001111b            ;length=1111b; AVAL=0; D=0; G=0
    r_gdt_code_base_high:       db 0

;data descriptor 16 bit
    r_gdt_data_limit_low:       dw 0FFFFh
    r_gdt_data_base_low:        dw 0
    r_gdt_data_base_middle:     db 0
    r_gdt_data_access:          db 10010010b        ;read/write; DPL=0; Present
    r_gdt_data_granularity:     db 00001111b        ;length=1111b; AVAL=0; D=0; G=0
    r_gdt_data_base_high:       db 0

gdt_size equ $ - gdt
gdt_str: 
    dw gdt_size - 1
    dd gdt

;Real mode interrupt vector table
r_idt:
    dw  0x03FF          ;Limit
    dd  0x00            ;Base

save_cr0:
    dd 0x00             ; Storage location for pmode CR0

save_32_sp:
    dd 0x00

save_16_sp:
    dw 0x00

Kern_32:
    mov ax, 0000000000010000b ; segment selector: RPL = 0, TI = 0, Descriptor Index = 2
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    mov edi, 0x0B8000            ; 0xB8000 is the beginning of video-memory in 0x3 video-mode
    mov esi, mess_in_32bit            ; the message which is going to be printed on the screen
    cld                         ; clear direction flag (DF is a flag used for string operations)

; Message-printing loop
.loop:                          
    lodsb                       ; load to al the next charachter located at [esi] address in memory (si is incremented automatically because the direction flag DF = 0)
    test al, al                 ; test al against zero
    jz boot_kern               ; exit if al is zero
    stosb                       ; otherwise move al to memory at address [edi]
    mov al, 7                   ; 7 is the so-called attribute of the symbol
    stosb                       ; move al to memory at address [edi]
    jmp .loop

mess_in_32bit db 'Hello World! in 32_bits', 0

cnt_flop:
    dd  KERN_OFFSET             ;2mb

boot_kern:

    ;push ((FLOP_BYTES_SECTOR * 61) - (kern_start - kernel_buffer))
    push ((FLOP_BYTES_SECTOR * 7) - (kern_start - mboot_struct))
    push KERN_OFFSET
    push kern_start
    call copy_kern

    ;mov eax, ((FLOP_BYTES_SECTOR * 61) - (kern_start - kernel_buffer))
    mov eax, ((FLOP_BYTES_SECTOR * 7) - (kern_start - mboot_struct))
    add [cnt_flop], eax
    mov eax, [cnt_flop]

    ;from 1 track, 1 head and 9 sector we read 46 sectors(== 1 track 10 sectors) 
    push 46         ;number of sectors are copied
    call floppy_read                    ;return 3 track 0 head 0 sector
        push FLOP_BYTES_SECTOR * 46                    ;number of bytes are copied
        push dword [cnt_flop]               ;!!!       ;the address to which we copy
        push kernel_buffer                             ;the address from which we copy
        call copy_kern

    mov eax, FLOP_BYTES_SECTOR * 46
    add [cnt_flop], eax
    mov eax, [cnt_flop]

    push FLOP_SECTORS                           ;number of sectors are copied
    call floppy_read                            ;return 4 track 1 head 0 sector
        push FLOP_BYTES_SECTOR * FLOP_SECTORS
        push dword [cnt_flop]
        push kernel_buffer
        call copy_kern

        ;jmp bbb

    mov ebx,  FLOP_MAX_SECTORS - 162         ;sectors left
    copy_cycle:
        mov eax, [cnt_flop]
        add eax, FLOP_BYTES_SECTOR * FLOP_SECTORS
        mov [cnt_flop], eax

        push FLOP_SECTORS       ;number of sectors are copied
        call floppy_read
            push dword FLOP_BYTES_SECTOR * FLOP_SECTORS
            push dword [cnt_flop]
            push kernel_buffer
            call copy_kern

        sub ebx, FLOP_SECTORS
        cmp ebx, FLOP_SECTORS
        jge copy_cycle
        cmp ebx, 0
        je .exit
    .end:
        mov eax, [cnt_flop]
        add eax, FLOP_BYTES_SECTOR * FLOP_SECTORS
        mov [cnt_flop], eax

        push ebx
        call floppy_read
        imul ebx, FLOP_BYTES_SECTOR
            push ebx
            push dword [cnt_flop]
            push kernel_buffer
            call copy_kern
    .exit:
            mov eax, [cnt_flop]
            add eax, ebx
            mov [cnt_flop], eax

    bbb:
    mov ebx, mboot_struct
    jmp KERN_OFFSET

floppy_read:
    push ebp
    mov ebp, esp

    push eax
    push ecx
    push edx
    push ebx

    mov bx, [ebp+8]            ;count of sectors are copied
    mov dl, 0
;------------ in real mode -------------------
    cli
    in al, 0x70
    or al, 0x80
    out 0x70, al

    jmp 0x18:go_16
    
BITS 16    
go_16:
    mov eax, 0x20
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    ;mov ss, ax

    lidt [r_idt]
    mov  [save_32_sp], esp

    mov eax, cr0
    mov [save_cr0], eax
    and eax, 0x7FFFFFFE
    mov cr0, eax

    jmp 0x00:to_16
;-----------------------
    to_16:
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    ;mov ss, ax

    mov ax, 0x7DF4
    mov sp, ax

    in al, 0x70
    and al, 0x7F
    out 0x70, al
    sti

flop_load_init:
    xor di, di
    mov di, bx
    mov al, 1
    mov bx, kernel_buffer
    mov ch, [track_num]
    mov dh, [head_num]
    mov cl, [sect_num]
    mov dl, 0

    flop_load:
        mov si, 3
    .read_rep:
        mov ah, 0x02  
        int 0x13
        jnc .rd_ok

        dec si
        jz .err
        xor ah, ah
        int 0x13
        jmp .read_rep
    .rd_ok:
        inc cl
        cmp cl, 18
        jng .rd_nxt

        mov cl, 0x01
        inc dh
        cmp dh, 0x01
        jng .rd_nxt

        xor dh, dh
        inc ch
        cmp ch, 80
        jge .err
    .rd_nxt:
        dec di
        jz .end_rd 
        add bx, 512
        jmp flop_load
    .end_rd:
        jmp .ext
    .err:
        push err_2h_int13h
        call BIOS_printf
    .ext:
        mov [track_num], ch
        mov [sect_num], cl
        mov [head_num], dh

;----------------- return in 32bit_mode ------------------
    cli 
    in al, 0x70
    or al, 0x80
    out 0x70, al

    mov [save_16_sp], sp

    ;mov eax, cr0
    ;or al, 0x01
    mov eax, [save_cr0]
    mov cr0, eax

    jmp dword 0x08:back_to_32
;---------------------------------------------------------
BITS 32
    back_to_32:
    mov eax, 0x10
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    ;mov ss, ax

    mov esp, [save_32_sp]

    pop ebx
    pop edx
    pop ecx
    pop eax

    mov esp, ebp
    pop ebp
ret 4

    copy_kern:
        push ebp
        mov ebp, esp

        push esi
        push edi
        push ecx

        mov esi, [ebp+8]
        mov edi, [ebp+12]
        mov ecx, [ebp+16]
        cld
        rep movsb

        pop ecx
        pop edi
        pop esi

        mov esp, ebp
        pop ebp
    ret 12

    kern_start:
    incbin "kernel.bin"
    ;... from kernel.asm
;===========================