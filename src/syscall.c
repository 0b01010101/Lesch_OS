
#include "syscall.h"

DEFN_SYSCALL1(in_byte, PORT_INPUT_BYTE, u16int)
DEFN_SYSCALL2(out_byte, PORT_OUTPUT_BYTE, u16int, u8int)

DEFN_SYSCALL0(get_vscreen, SYS_GET_VSCREEN)
DEFN_SYSCALL0(monitor_clear, SYS_MONIT_CLEAR)
DEFN_SYSCALL0(get_asc2, SYS_GET_ASC2)
DEFN_SYSCALL0(get_keyboard, SYS_GET_KEYB)
DEFN_SYSCALL0(proc_destroy, SYS_PROC_DESTROY);
DEFN_SYSCALL1(del_vscreen, SYS_DEL_VSCREEN, vscreen_s*)
DEFN_SYSCALL1(get_keyb_buff, SYS_KEYB_BUFF, u32int)
DEFN_SYSCALL1(mpfile_remove, SYS_MPFIL_REMV, mpfile_s*)
DEFN_SYSCALL2(vwrite_char, SYS_VWRITE_CHAR, vscreen_s*, u8int)
DEFN_SYSCALL2(vwrite_str, SYS_VWRITE_STR, vscreen_s*, u8int*)
DEFN_SYSCALL2(vwrite_dec, SYS_VWRITE_DEC, vscreen_s*, u32int)
DEFN_SYSCALL2(vwrite_hex, SYS_VWRITE_HEX, vscreen_s*, u32int)
DEFN_SYSCALL2(mpfile_set, SYS_MPFIL_SET, u8int*, u32int*);
DEFN_SYSCALL2(mpfile_get, SYS_MPFIL_GET, u8int*, u32int*);
DEFN_SYSCALL1(umalloc_align, SYS_UMALL_ALI, u32int);
DEFN_SYSCALL1(shell_handler, SYS_SHELL_HANDLER, u8int *);

DEFN_SYSCALL0(tty16_swap_buff, SYS_TTY16_SWPBUFF);
DEFN_SYSCALL1(tty16_put_char, SYS_TTY16_CHAR, u8int);
DEFN_SYSCALL3(print16, SYS_PRINT_16, u8int *, u32int, u32int);

DEFN_SYSCALL3(socket, SYS_SOCKET, u8int, u8int, u8int);
DEFN_SYSCALL2(sock_listen, SYS_SOCK_LISTN, u16int, u16int);
DEFN_SYSCALL1(sock_close, SYS_SOCK_CLOSE, u16int);
DEFN_SYSCALL3(sock_bind, SYS_SOCK_BIND, u16int, void*, u16int);
DEFN_SYSCALL1(sock_accept, SYS_SOCK_ACCPT, u16int);
DEFN_SYSCALL2(sock_recv, SYS_SOCK_RECV, socket_s*, u8int*);
DEFN_SYSCALL2(ssock_send, SYS_SOCK_SEND, socket_s*, u8int);

void *calls_tabl[NUM_CLL] = { &in_byte, &out_byte, &get_vscreen, &del_vscreen, &monitor_clear, &vwrite_str, \
                            &vwrite_dec, &vwrite_hex, &vwrite_char, &get_keyboard, &get_keyb_buff, &get_asc2, \
                            &proc_destroy, &mpfile_set, &mpfile_get, &mpfile_remove, &umalloc_align, &print16, \
                            &tty16_put_char, &tty16_swap_buff, &shell_handler };

void syscall_handler(registers_t regs) {

    if(regs.eax >= NUM_CLL) {
        return;
    }

    void *sscll = calls_tabl[regs.eax];
    regs.eax = syscall_call(sscll);

    return;
}

void init_syscall_handlers(void) {

    register_interrupt_handler(48, syscall_handler);
    return;
}