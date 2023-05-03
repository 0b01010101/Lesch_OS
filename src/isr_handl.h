#ifndef ISR_HANDL_H
#define ISR_HANLD_H

#define     ISR0            0
#define     ISR1            1
#define     ISR2            2
#define     ISR3            3
#define     ISR4            4
#define     ISR5            5
#define     ISR6            6
#define     ISR7            7
#define     ISR8            8
#define     ISR9            9
#define     ISR10           10
#define     ISR11           11
#define     ISR12           12
#define     ISR13           13
#define     ISR14           14
#define     ISR15           15
#define     ISR16           16
#define     ISR17           17
#define     ISR18           18

#define     EXT_BIT         (1 << 0)
#define     IDT_BIT         (1 << 1)
#define     TI_BIT          (1 << 2)
#define     ERR_CODE_MASK   0xFFF8;

#include "common.h"
#include "isr.h"
#include "schedul.h"
#include "process.h"

void init_isr_handlers();
void isr6_handl(registers_t regs);
void isr14_handl(registers_t regs);

#endif