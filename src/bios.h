#ifndef     BIOS_H
#define     BIOS_H

#include "common.h"
#include "tables_descript.h"
#include "help_func.h"
#include "isr.h"

#define REBASE(x)   (void *)(0x7C00 + (void *)x - (u32int)bits32to16)
#define BIOS_GRAPH  0x10

extern gdt_entr gdt_table[];
extern void *asm_gdt_table;
extern void *asm_gdt_ptr;
extern void *asm_idt_ptr;
extern void *asm_out_reg_ptr;
extern void *asm_in_reg_ptr;
extern void *asm_intnum_ptr;
extern void bits32to16_end();
extern void bits32to16();

void (*rebase_32to16)(void) = (void *)0x7C00;
void bios32_init(void);
void bios32_serv(u8int num_int, regs_16 *reg_in, regs_16 *reg_out);

#endif