#include "common.h"
#include "tables_descript.h"

extern void gdt_flush(u32int);
extern void idt_flush(u32int);
extern void tss_flush(u32int);

static void init_gdt();
static void gdt_set(s32int, u32int, u32int, u8int, u8int);
static void init_idt();
static void idt_set(u8int, u32int, u16int, u8int);
static void tss_set(s32int num, u32int ss0, u32int esp0);

u32int get_tss_esp0(void);


struct gdt_str      gdt_table[10];
struct gdt_ptr_str  gdt_ptr;
struct idt_str      idt_table[256];
struct idt_ptr_str  idt_ptr;

tss_entr            tss;


void init_tables_descr() {

    init_gdt();     // Initialise the global descriptor table.
    init_idt();
    return;
}

static void init_gdt() {

    gdt_ptr.size = (sizeof(struct gdt_str) * 10) -1;
    gdt_ptr.base = (u32int)&gdt_table;

    gdt_set(0, 0, 0, 0, 0);
    gdt_set(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
    gdt_set(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
    gdt_set(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
    gdt_set(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment

    tss_set(5, 0x10, init_esp);           //TSS segment

    gdt_flush((u32int)&gdt_ptr);
    tss_flush(0x28);

    return;
}

void tss_set(s32int num, u32int ss0, u32int esp0) {

    memset(&tss, 0, sizeof(tss_entr));

    /*kern*/
    tss.ss0  = ss0;
    tss.esp0 = esp0;
    /*CPL*/
    tss.cs   = 0x08;
    tss.ss   = 0x10;
    tss.es   = 0x10;
    tss.ds   = 0x10;
    tss.gs   = 0x10;
    tss.fs   = 0x10;

    tss.iomap = 0xFF;
    tss.iomap_offset = (u16int) ((u32int) &tss.iomap - (u32int) &tss);

    u32int base  = (u32int) &tss;
    u32int limit = sizeof(tss) - 1;

    tss_seg *tss_tmp = (tss_seg*)&gdt_table[num];

    /*set base*/
    tss_tmp->base_15_0  = base & 0xFFFF;
    tss_tmp->base_23_16 = (base >> 16) & 0xFF;
    tss_tmp->base_31_24 = (base >> 24) & 0xFF;
    /*set limit*/
    tss_tmp->limit_15_0  = limit & 0xFFFF;
    tss_tmp->limit_19_16 = (limit >> 16) & 0x0F;
    /*set bits*/
    tss_tmp->present = 1;
    tss_tmp->sys = 0;
    tss_tmp->DPL = 0;
    tss_tmp->type = 9;
    tss_tmp->AVL = 0;
    tss_tmp->allways_zero = 0;
    tss_tmp->gran = 0;

    return;
}

u32int get_tss_esp0(void) {
    return tss.esp0;
}

static void gdt_set(s32int num, u32int base, u32int limit, u8int access,  u8int gran) {

    gdt_table[num].base_low    = (base & 0xFFFF);
    gdt_table[num].base_middle = (base >> 16) & 0xFF;

    gdt_table[num].base_high   = (base >> 24) & 0xFF;
    gdt_table[num].limit_low   = (limit & 0xFFFF);
    gdt_table[num].granularity = (limit >> 16) & 0x0F;

    gdt_table[num].granularity |= gran & 0xF0;
    gdt_table[num].access      = access;

   return;
}

static void init_idt() {

    outb(0x20, 0x11);    // Remap the irq table
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x00);
    outb(0xA1, 0x00); 

    idt_ptr.size = (sizeof(struct idt_str) * 256) -1;
    idt_ptr.base = (u32int)&idt_table;

    memset(&idt_table, 0, sizeof(struct idt_str)*256);

    idt_set(0, (u32int)isr0, 0x08, 0x8E);
    idt_set(1, (u32int)isr1, 0x08, 0x8E);
    idt_set(2, (u32int)isr2, 0x08, 0x8E);
    idt_set(3, (u32int)isr3, 0x08, 0x8E);
    idt_set(4, (u32int)isr4, 0x08, 0x8E);
    idt_set(5, (u32int)isr5, 0x08, 0x8E);
    idt_set(6, (u32int)isr6, 0x08, 0x8E);
    idt_set(7, (u32int)isr7, 0x08, 0x8E);
    idt_set(8, (u32int)isr8, 0x08, 0x8E);
    idt_set(9, (u32int)isr9, 0x08, 0x8E);
    idt_set(10, (u32int)isr10, 0x08, 0x8E);
    idt_set(11, (u32int)isr11, 0x08, 0x8E);
    idt_set(12, (u32int)isr12, 0x08, 0x8E);
    idt_set(13, (u32int)isr13, 0x08, 0x8E);
    idt_set(14, (u32int)isr14, 0x08, 0x8E);
    idt_set(15, (u32int)isr15, 0x08, 0x8E);
    idt_set(16, (u32int)isr16, 0x08, 0x8E);
    idt_set(17, (u32int)isr17, 0x08, 0x8E);
    idt_set(18, (u32int)isr18, 0x08, 0x8E);
    idt_set(19, (u32int)isr19, 0x08, 0x8E);
    idt_set(20, (u32int)isr20, 0x08, 0x8E);
    idt_set(21, (u32int)isr21, 0x08, 0x8E);
    idt_set(22, (u32int)isr22, 0x08, 0x8E);
    idt_set(23, (u32int)isr23, 0x08, 0x8E);
    idt_set(24, (u32int)isr24, 0x08, 0x8E);
    idt_set(25, (u32int)isr25, 0x08, 0x8E);
    idt_set(26, (u32int)isr26, 0x08, 0x8E);
    idt_set(27, (u32int)isr27, 0x08, 0x8E);
    idt_set(28, (u32int)isr28, 0x08, 0x8E);
    idt_set(29, (u32int)isr29, 0x08, 0x8E);
    idt_set(30, (u32int)isr30, 0x08, 0x8E);
    idt_set(31, (u32int)isr31, 0x08, 0x8E);

    idt_set(32, (u32int)irq0, 0x08, 0x8E);
    idt_set(33, (u32int)irq1, 0x08, 0x8E);
    idt_set(34, (u32int)irq2, 0x08, 0x8E);
    idt_set(35, (u32int)irq3, 0x08, 0x8E);
    idt_set(36, (u32int)irq4, 0x08, 0x8E);
    idt_set(37, (u32int)irq5, 0x08, 0x8E);
    idt_set(38, (u32int)irq6, 0x08, 0x8E);
    idt_set(39, (u32int)irq7, 0x08, 0x8E);
    idt_set(40, (u32int)irq8, 0x08, 0x8E);
    idt_set(41, (u32int)irq9, 0x08, 0x8E);
    idt_set(42, (u32int)irq10, 0x08, 0x8E);
    idt_set(43, (u32int)irq11, 0x08, 0x8E);
    idt_set(44, (u32int)irq12, 0x08, 0x8E);
    idt_set(45, (u32int)irq13, 0x08, 0x8E);
    idt_set(46, (u32int)irq14, 0x08, 0x8E);
    idt_set(47, (u32int)irq15, 0x08, 0x8E);
    idt_set(48, (u32int)isr48, 0x08, 0xEF);         //syscall

    idt_flush((u32int)&idt_ptr);
    init_isr_handlers();                         //in "isr_handl.c";
    init_syscall_handlers();                     //in "syscall.c";
    
    return;
}

static void idt_set(u8int num, u32int base, u16int sel, u8int flags) {

    idt_table[num].base_lo = base & 0xFFFF;
    idt_table[num].base_hi = (base >> 16) & 0xFFFF;
    idt_table[num].sel     = sel;
    idt_table[num].always0 = 0;
    idt_table[num].flags   = flags /* | 0x60 (for user)*/;

    return;
}


