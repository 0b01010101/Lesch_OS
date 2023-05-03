#include "isr.h"
#include "monitor.h"
#include "common.h"

isr_t interrupt_handlers[256];

__attribute__ ((target ("no-sse")))             //or "gcc -mno-sse"
void isr_handler(registers_t regs) {

/*
    monitor_str_write("ISR_int: ");
    write_dec(regs.int_num);
    monitor_char_put('\n');
*/
    if(interrupt_handlers[regs.int_num] != 0) {

        isr_t handler = interrupt_handlers[regs.int_num];
        handler(regs);
    }
    return;
}

 __attribute__ ((target ("no-sse")))
void irq_handler(registers_t regs) {
/*
    monitor_str_write("IRQ_int: ");
    write_dec(regs.int_num);
    monitor_char_put('\n');
*/
    if(regs.int_num >= 40) {

        outb(0xA0, 0x20);       // Send reset signal to slave;
    }
    outb(0x20, 0x20);

    if(interrupt_handlers[regs.int_num] != 0) {

        isr_t handler = interrupt_handlers[regs.int_num];
        handler(regs);
    }
    return;
}

void register_interrupt_handler(u8int n, isr_t handler) {

        interrupt_handlers[n] = handler;
        return;
}