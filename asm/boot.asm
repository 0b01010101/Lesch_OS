BITS 16
org 7C00h

BOOTS       equ 0x0050
SECTR_COUNT equ 61

start:
    jmp dword 0x0000:entr
;================================================
entr:  
    ;mov ax, 7C00h
    xor ax, ax
    mov ds, ax
    mov ax, 9000h
    cli
    mov ss, ax
    mov sp,0FFF0h
    sti

    mov ax, 0x0003  ; ah=0 means clear screen and setup video mode, al=3 means text mode, 80x25 screen, CGA/EGA adapter, etc.
    int 10h

    mov byte [id_disk], dl
    push mess_boot_loading
    call BIOS_printf

; Set destination segment to load kernel to
    ;mov ax, BOOTS
    ;mov es, ax

;Resset drive
    _disk_res:
        mov ah, 0
        int 13h
    jc _disk_res

    push dword [id_disk]      ; push disk identifier
    push dword 2              ; LBA of the 1-st sector to load
    push SECTR_COUNT          ; push count of sectors to load
    push kernel_buffer          ; push memory address at(in) which sectors are to be loaded
    call BIOS_LoadSectors

    xor ax, ax
    mov al, [track_num]
    xor dx, dx
    mov dl, [head_num]
    xor bx, bx
    mov bl, [sect_num]

    ; Set data segment
    push es
    pop ds
    ; Pass control to kernel
    push ds
    push word kernel_buffer
    retf
;-----------------------------------------------------------------------
    BIOS_LoadSectors:
        push bp
        mov bp, sp

        push es
        push ax
        push bx
        push dx

        mov ax, ds
        mov es, ax

        ;mov ax, 62
        mov di, [bp+6]
        ;mov [sect_cnt], ax
        mov al, 0x01
        mov bx, kernel_buffer       ;== 0x7E00
        xor ch,ch
        mov cl, 0x02
        xor dx, dx

        load_sect:
            mov si, 0x03      ; count of errors (max 3)
        .read_repeat:
            mov ah, 0x02      ; (int 0x13) Read Sectors From Drive function
            int 0x13
            jnc .read_ok

            dec si
            jz .error
            xor ah, ah
            int 0x13         ;reset disk
            jmp .read_repeat
        .read_ok:
            inc cl           ;sector
            cmp cl, 18
            jng .read_next

            mov cl, 0x01
            inc dh              ;head
            cmp dh, 0x01
            jng .read_next

            xor dh, dh
            inc ch              ;track
            cmp ch, 80
            jge .error
        .read_next:
            dec di
            jz .end_read
            add bx, 512
            jmp load_sect
        .end_read:
            jmp .exit
        .error: 
            push err_2h_int13h
            call BIOS_printf
        .exit:
        mov [track_num], ch
        mov [head_num], dh
        mov [sect_num], cl
        pop dx
        pop bx
        pop ax
        pop es

        mov sp, bp
        pop bp
    ret 12

        BIOS_GetDiskParametrs:
; Determines parameters of a specified disk.
; Parameters: word [bp + 4] - disk identifier
; Returns: dl - overall number of disks in the system
;          dh - maximum head index
;          ch - maximum cylinder index
;          cl - number of sectors per a track
; Remarks: floppy disk 1.44 MB has 2 heads (0 and 1), 80 tracks (0...79) and 18 sectors(1...18) per a track
            push bp
            mov bp, sp
            push es

            xor ax, ax
            mov es, ax
            xor di, di

            mov dl, [bp+4]
            mov ah, 0x08        ; read drive parameters
            int 13h

            pop es
            mov sp, bp
            pop bp
        ret 

        LBA_to_CHS:
            push bp
            mov bp, sp
            push ax

            ; dx:ax = LBA
            mov dx, [bp + 10]
            mov ax, [bp + 8]

            movzx cx, byte [ebp + 6] ; cx = SPT

            div cx            ; divide dx:ax (LBA) by cx (SPT) (AX = quotient, DX = remainder)
            mov cl, dl        ; CL = SECTOR = remainder
            inc cl            ; sectors are indexed starting from 1

            div byte [bp + 4] ; AL = LBA % HPC; AH = remainder
            mov dh, ah        ; DH = HEAD = remainder
            mov ch, al        ; CH = CYLINDER = quotient

            pop ax
            mov sp, bp
            pop bp
        ret 8

        BIOS_printf:
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

;------------------------------------------------------------------------
    id_disk dw 0x0000
    sect_cnt:   dw 0x00
    head_num:   db 0x00
    track_num:  db 0x00
    sect_num:   db 0x00
    mess_boot_loading db 'Starting bootloader...', 0x0d, 0x0a, 0
    err_2h_int13h:  db 'ERROR 02h int 13h!', 0x0d, 0x0a, 0
   

    finish:
        times 510 - ($-$$) db 0
        db 55h, 0AAh

%include "asm\kern_boot.asm"
