
#include "timer.h"

void init_timer(u32int need_frequency) {

    u32int divisor;
    u8int low;
    u8int high;

    register_interrupt_handler(IRQ0, &timer_callback);

    divisor = 1193180 / need_frequency;
    low = (char) (divisor & 0xFF);
    high = (char) ((divisor >> 8) & 0xFF);

    outb(0x43, 0x36);
    outb(0x40, low);
    outb(0x40, high);

    rnd_set(0xFFFF);
    frequency = need_frequency;

    return;
}

void reset_timer(void) {
    
    u32int div = 1193180 / frequency;
    u8int low = (u8int) (div & 0xFF);
    u8int high = (u8int) ((div >> 8) & 0xFF);

    outb(0x43, 0x36);
    outb(0x40, low);
    outb(0x40, high);

    return;
}

void rnd_set(u16int count) {                //set random number (0 - 0xffff);

    outb(0x43, 0xb6);
    outb(0x42, count & 0x00FF);
    outb(0x42, (count & 0xFF00) >> 8);
    outb(0x61, (inb(0x61) | 1));

    return;
}

u32int rnd_get(void) {                         //get random number (0 - 0xffff);

    u32int i;

    outb(0x43, 0x86);
    i = inb(0x42);
    i = (inb(0x42) << 8) + i;

    return i;
}

vscreen_s vs = {0};

void __attribute__((optimize("O0"))) timer_callback(registers_t regs) {

    vs.cur_x = 73;
    vs.cur_y = 23;
    vwrite_str(&vs, "??");
    vs.cur_x = 73;
    vwrite_str(&vs, "m");
    vwrite_dec(&vs, get_multitask());
    //if(get_multitask()) {
    if(!get_multitask()) {
        //wprint("ID(%d), t(%d), %x\n", curr_thread->id, multitask, &multitask);
        vwrite_str(&vs, "[");
        vwrite_dec(&vs, curr_thread->id);
        vwrite_str(&vs, "]");
        switch_task();
    }

    tick++;
    if(tick == TIMER_FREQ) {
        tick = 0;
        sec++;
    }
    
    if(sec > 59) {
        sec = 0;
        min++;
    }

    if(min > 59) {
        min = 0;
        hour++;
    }
    return;
}

