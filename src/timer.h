#ifndef TIMER_H
#define TIMER_H

#include "common.h"
#include "isr.h"
#include "monitor.h"
#include "schedul.h"

u32int tick = 0;
u8int sec   = 0;
u8int min   = 0;
u8int hour  = 0;
u32int  frequency;

extern thread_s *curr_thread;

#define TIMER_FREQ      1000

extern void switch_task(void);

void init_timer(u32int frequency);
void reset_timer(void);
void rnd_set(u16int count);
u32int rnd_get(void);
static void timer_callback(registers_t regs);

#endif