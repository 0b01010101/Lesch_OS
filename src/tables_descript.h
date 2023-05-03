#ifndef TABLES_DESCRIPT_H
#define TABLES_DESCRIPT_H

#include "common.h"

struct gdt_str {
    u16int limit_low;           // Младшие 16 битов граничного значения limit.
    u16int base_low;            // Младшие 16 битов адресной базы.
    u8int  base_middle;         // Следующие 8 битов адресной базы.
    u8int  access;              // Флаги доступа, определяющие в каком кольце можно использовать этот сегмент.
    u8int  granularity;
    u8int  base_high;           // Последние 8 битов адресной базы.
} __attribute__((packed));

struct gdt_ptr_str {
    u16int size;
    u32int base;
} __attribute__((packed));

struct idt_str {
    u16int base_lo;             // Младшие 16 битов адреса, куда происходи переход в случае возникновения прерывания.
    u16int sel;                 // Переключатель сегмента ядра.
    u8int  always0;             // Это значение всегда должно быть нулевым.
    u8int  flags;               // More flags. See documentation.
    u16int base_hi;             // Старшие 16 битов адреса, куда происходи переход.
} __attribute__((packed));

struct idt_ptr_str {
    u16int size;
    u32int base;
} __attribute__((packed));

struct tss_str {
    u16int  limit_15_0;     //
    u16int  base_15_0;      //
    u8int   base_23_16;     //

    u8int   type:4;         //Тип сегмента
    u8int   sys:1;          //Системный сегмент
    u8int   DPL:2;          //Уровень привилегий сегмента
    u8int   present:1;      //Бит присутствия

    u8int   limit_19_16:4;  //Биты 19-16 лимита
    u8int   AVL:1;          //Зарезервирован
    u8int   allways_zero:2; //Всегда нулевые
    u8int   gran:1;         //Бит гранулярности

    u8int   base_31_24;     //
} __attribute__((packed));

struct tss_ptr_str {
    u32int  prev_tss;
    u32int  esp0;
    u32int  ss0;
    u32int  esp1;
    u32int  ss1;
    u32int  esp2;
    u32int  ss2;
    u32int  cr3;
    u32int  eip;
    u32int  eflags;
    u32int  eax;
    u32int  ecx;
    u32int  edx;
    u32int  ebx;
    u32int  esp;
    u32int  ebp;
    u32int  esi;
    u32int  edi;
    u32int  es;
    u32int  cs;
    u32int  ss;
    u32int  ds;
    u32int  fs;
    u32int  gs;
    u32int  ldrt;
    u16int  task_flags;
    u16int  iomap_offset;   //Смещение от начала TSS до I/O map
    u8int   iomap;          //имитируюет карту I/O
} __attribute__((packed));

typedef struct gdt_str gdt_entr;
typedef struct gdt_ptr_str gdt_pt;
typedef struct idt_str idt_entr;
typedef struct idt_ptr_str idt_pt;
typedef struct tss_str tss_seg;
typedef struct tss_ptr_str tss_entr;

void init_tables_descr();

extern void init_isr_handlers();
extern void init_syscall_handlers();
extern u32int init_esp;

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

extern void isr48();        //syscall;

#endif
