#include "keyboard.h"

u8int asc_buff[] = {0, 0, '1', '2', '3', '4', '5', '6', '7',
'8', '9', '0', '-', '=', 8,'\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
'[', ']','\n', 0,'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'',
'`',0,'\\', 'z', 'x', 'c', 'v','b', 'n', 'm', ',', '.', '/',0,'*',0,' ',0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,'-', 0, 0, 0,'+', 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

char asc_buff_sh[] = {0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')','_',
'+', 8, '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X',
'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

u8int key_buff[KEYB_BUFF_SIZE] = {0};   //Keyboard buffer
u8int shift = 0;
u32int kb_start = 0;                    //Begin of keyboard buffer
u32int kb_end = 0;                      //End of keyboard buffer


void init_keyboard(void) {

    register_interrupt_handler(IRQ1, &keyboard_handler);
}

__attribute__ ((target ("no-sse")))
void keyboard_handler (registers_t) {

    u8int stat = 0;

    stat = inb(0x64);
    if(stat & IS_READ) {

        if(kb_end >= KEYB_BUFF_SIZE) {

            kb_end = 0;
        }
        key_buff[kb_end] = inb(0x60);
        kb_end++;
    }
    return;
}

__attribute__ ((target ("no-sse")))
u8int get_keyboard(void) {
    
    u8int res = 0;

    if(kb_start != kb_end) {

        if(kb_start >= KEYB_BUFF_SIZE) {
            kb_start = 0;
        }
        res = key_buff[kb_start];
        kb_start++;
    }
    else { 
        res = 0;
    }
    return res;
}

__attribute__ ((target ("no-sse")))
u8int get_keyb_buff(u32int indx) {

    return key_buff[indx];
}


u8int get_asc2() {

    u8int asc = 0;
    u8int res = get_keyboard();

        if(res == 0x36 || res == 0x2A) {

            shift = 1;
            }

        else if(res == (0x36+0x80) || res == (0x2A+0x80)) {

            shift = 0;
            }

        else if(res < 0x48) {

                if(shift) {
                    asc = asc_buff_sh[res];
                } else {
                    asc = asc_buff[res];
                }
                return asc;
            }
        else {
            if(res >= 0x48 && res <= 0x50) {
                asc = res;
            }
        }
    return asc;
}