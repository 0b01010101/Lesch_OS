
#include "bios.h"

idt_pt  real_gdt_ptr;
idt_pt  real_idt_ptr;

void bios32_init(void) {

    gdt_set(6, 0, 0xFFFFFFFF, 0x9A, 0x0F);
    gdt_set(7, 0, 0xFFFFFFFF, 0x92, 0x0F);

    real_gdt_ptr.base = (u32int)gdt_table;
    real_gdt_ptr.size = sizeof(gdt_table) - 1;

    real_idt_ptr.base = 0;
    real_idt_ptr.size = 0x03FF;

    return;
}

void bios32_serv(u8int num_int, regs_16 *reg_in, regs_16 *reg_out) {

    u32int size;
    void *tmp;
    void *code_base = (void *)0x7C00;

    memcpy(&asm_gdt_table, gdt_table, sizeof(gdt_table));
    real_gdt_ptr.base = (u32int)REBASE((&asm_gdt_table));

    memcpy(&asm_gdt_ptr, &real_gdt_ptr, sizeof(real_gdt_ptr));
    memcpy(&asm_idt_ptr, &real_idt_ptr, sizeof(real_idt_ptr));

    memcpy(&asm_in_reg_ptr, reg_in, sizeof(regs_16));
    memcpy(&asm_intnum_ptr, &num_int, sizeof(u8int));
    tmp = REBASE(&asm_in_reg_ptr);

    size =  (u32int)bits32to16_end - (u32int)bits32to16;
    memcpy(code_base, bits32to16, size);

    if(size > 4096) {
        monitor_str_write("!ERROR! bits32to16(): enough memory\n");
        goto end;
    } 
    rebase_32to16();

    tmp =  REBASE(&asm_out_reg_ptr);
    memcpy(reg_out, tmp, sizeof(regs_16));

end:
    //init_gdt();
    //init_idt();
    gdt_flush((u32int)&gdt_ptr);
    idt_flush((u32int)&idt_ptr);

    return;
}