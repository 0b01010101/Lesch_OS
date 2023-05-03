#ifndef     KEYBOARD_H
#define     KEYBOARD_H    

#include "isr.h"
#include "common.h"

#define     KEYB_BUFF_SIZE      32

#define     IS_READ             (1<<0)
#define     IS_WRITE            (1<<1)
#define     IS_RESET            (1<<2)
#define     IS_CMD              (1<<3)
#define     IS_LOCK             (1<<4)
#define     IS_MOUSE            (1<<5)
#define     IS_TIMEOUT          (1<<6)
#define     IS_ERROR            (1<<7)

void init_keyboard(void);
void keyboard_handler(registers_t regs);
u8int get_keyboard(void);
u8int get_keyb_buff(u32int indx);

#endif