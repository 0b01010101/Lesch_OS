
#ifndef     SYSCALLS_H
#define     SYSCALLS_H

#include    "common.h"
#include    "socket.h"
#include    "isr.h"
#include    "monitor.h"
#include    "io_disp.h"

#define     NUM_CLL     30           //number of system functions in table (calls_tabl);
#define     SYSCALL     48          //number of syscall;

#define     PORT_INPUT_BYTE     0
#define     PORT_OUTPUT_BYTE    1

#define     SYS_GET_VSCREEN     2
#define     SYS_DEL_VSCREEN     3
#define     SYS_MONIT_CLEAR     4
#define     SYS_VWRITE_STR      5
#define     SYS_VWRITE_DEC      6
#define     SYS_VWRITE_HEX      7
#define     SYS_VWRITE_CHAR     8
#define     SYS_GET_KEYB        9
#define     SYS_KEYB_BUFF       10
#define     SYS_GET_ASC2        11
#define     SYS_PROC_DESTROY    12
#define     SYS_MPFIL_SET       13
#define     SYS_MPFIL_GET       14
#define     SYS_MPFIL_REMV      15
#define     SYS_UMALL_ALI       16
#define     SYS_PRINT_16        17
#define     SYS_TTY16_CHAR      18
#define     SYS_TTY16_SWPBUFF   19
#define     SYS_SHELL_HANDLER   20
#define     SYS_SOCKET          21
#define     SYS_SOCK_LISTN      22
#define     SYS_SOCK_CLOSE      23
#define     SYS_SOCK_BIND       24
#define     SYS_SOCK_ACCPT      25
//#define     SYS_SOCK_CONNECT    26
#define     SYS_SOCK_RECV       27
#define     SYS_SOCK_SEND       28

void init_syscall_handlers(void);
void syscall_handler(registers_t regs);

extern u32int syscall_call(void *sscll);
extern s32int shell_handler(u8int *);
//////////////////////////////////////////////////////////////////////
/*------------------------------------------------------------------
 *        Macro definitions of system calls
 *----------------------------------------------------------------*/

/*---- Without parameters---------------------------------------------*/
#define DECL_SYSCALL0(func)         int syscall_##func(void)

#define DEFN_SYSCALL0(func, num) \
\
int syscall_##func(void) {\
\
    int ret = 0;\
    asm volatile ("int $0x30":"=a"(ret):"a"(num));\
    return ret;\
}
/////////////////////////////////////////////////////////////////////
/*---- One parameter ---------------------------------------------*/
#define DECL_SYSCALL1(func, p1)     int syscall_##func(p1)

#define DEFN_SYSCALL1(func, num, P1) \
\
int syscall_##func(P1 p1) {\
\
    int ret = 0;\
    asm volatile ("int $0x30":"=a"(ret):"a"(num), "b"(p1));\
    return ret;\
}
/////////////////////////////////////////////////////////////////////
/*---- Two parameters ---------------------------------------------*/
#define DECL_SYSCALL2(func, p1, p2)  int syscall_##func(p1, p2)

#define DEFN_SYSCALL2(func, num, P1, P2) \
\
int syscall_##func(P1 p1, P2 p2) {\
\
    int ret = 0;\
    asm volatile ("int $0x30":"=a"(ret):"a"(num), "b"(p1), "c"(p2));\
    return ret;\
}
/////////////////////////////////////////////////////////////////////
/*---- Three parameters --------------------------------------------*/
#define DECL_SYSCALL3(func, p1, p2, p3) int syscall_#func(p1, p2, p3)

#define DEFN_SYSCALL3(func, num, P1, P2, P3)\
\
int syscall_##func(P1 p1, P2 p2, P3 p3) {\
\
    int ret = 0;\
    asm volatile ("int $0x30":"=a"(ret):"a"(num), "b"(p1), "c"(p2), "d"(p3));\
    return ret;\
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DECL_SYSCALL1(in_byte, u16int);
DECL_SYSCALL2(out_byte, u16int, u8int);

DECL_SYSCALL0(get_vscreen);
DECL_SYSCALL0(monitor_clear);
DECL_SYSCALL0(proc_destroy);
DECL_SYSCALL1(del_vscreen, vscreen_s*);
DECL_SYSCALL1(umalloc_align, u32int);
DECL_SYSCALL2(vwrite_str, vscreen_s*, u8int*);
DECL_SYSCALL2(vwrite_dec, vscreen_s*, u32int);
DECL_SYSCALL2(vwrite_hex, vscreen_s*, u32int);
DECL_SYSCALL2(mpfile_set, u8int*, u32int*);
DECL_SYSCALL2(mpfile_get, u8int*, u32int*);

//DECL_SYSCALL3(print16, u8int*, u32int, u32int);

#endif