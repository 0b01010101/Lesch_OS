#ifndef ISR_H
#define ISR_H

#include "common.h"

#define IRQ0     32
#define IRQ1     33
#define IRQ2     34
#define IRQ3     35
#define IRQ4     36
#define IRQ5     37
#define IRQ6     38
#define IRQ7     39
#define IRQ8     40
#define IRQ9     41
#define IRQ10    42
#define IRQ11    43
#define IRQ12    44
#define IRQ13    45
#define IRQ14    46
#define IRQ15    47

extern void write_hex(u32int n);
extern void write_dec(u32int n);

typedef struct registers {
    u32int ds;
    u32int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    u32int int_num, err_code;                       // Номер прерывания и код ошибки (если он предоставляется)
    u32int eip, cs, eflags, useresp, ss;            // Значения автоматически помещаются процессором в стек.
} registers_t;

typedef struct registers16 {
    u16int  di;
    u16int  si;
    u16int  bp;
    u16int  sp;
    u16int  bx;
    u16int  dx;
    u16int  cx;
    u16int  ax;

    u16int  ds;
    u16int  es;
    u16int  fs;
    u16int  gs;
    u16int  ss;
    u16int  eflags;
} regs_16;

typedef void (*isr_t)(registers_t);

void register_interrupt_handler(u8int n, isr_t handler);

#endif


