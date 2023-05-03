#include "isr_handl.h"
#include "monitor.h"

void init_isr_handlers() {
    register_interrupt_handler(6, isr6_handl);
    register_interrupt_handler(14, isr14_handl);
}

void isr6_handl(registers_t regs) {
    monitor_str_write("isr: ");
    write_dec(regs.int_num);
    monitor_str_write(" | code: ");
    write_hex(regs.err_code);
    monitor_str_write(" | cs:eip__");
    write_hex(regs.cs);
    monitor_str_write(":");
    write_hex(regs.eip);
    monitor_char_put('\n');
    while(1);
}

void isr14_handl(registers_t regs) {

    u32int addr_fault = 0;

    asm volatile ("mov %cr2, %eax");
    asm volatile ("mov %%eax, %0":"=a"(addr_fault));

    u32int present = !(regs.err_code & 0x01);   /* Страница отсутствует в памяти */
    u32int user    = regs.err_code & 0x04;      /* Обращение с нарушением привилегий */
    u32int rw      = regs.err_code & 0x02;      /* Страница только для чтения */
    u32int reserv  = regs.err_code & 0x08;      /* Перезаписан зарезервированный флаг */

    monitor_str_write("\n\nPage Fault: #");
    write_dec(regs.int_num);

    if(present) {

        monitor_str_write("|NOT PRESENT,");
    }

    if(user) {

        monitor_str_write("USER MODE,");
    }

    if(rw) {

        monitor_str_write("READ ONLY,");
    }

    if(reserv) {

        monitor_str_write("WRITING TO RESERVED BITS, ");
    }
    wprint(" cs(%x) eip(%x) esp(%x)\nlin.comm.addr.(%x),PID(%d),TID(%d)\n", regs.cs, regs.eip, regs.esp, addr_fault, get_curr_proc()->pid, get_curr_thread()->id);

    while(1);
}

