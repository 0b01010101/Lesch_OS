#ifndef     TTY_H
#define     TTY_H

#include    "common.h"
#include    "schedul.h"
#include    "memory.h"

#define     SCRN_WIDTH  80
#define     SCRN_HEIGHT 25

#define     COL_BLACK   0
#define     COL_BLUE    9

struct tty16 {

    list_item_s     list_item;
    //u8int           name[32];
    u32int          id;
    u8int           x0;
    u8int           y0;
    u8int           xmax;
    u8int           ymax;
    u8int           width;
    u8int           height;
    u8int           cur_x;
    u8int           cur_y;
    u8int           col_back;
    u8int           col_txt;
    u8int           bs_xmin;
    u16int          *vbuff;
};

typedef struct tty16 tty16_s;

tty16_s *tty16_init(void);
void tty16_swap_buff(void);
void tty16_set_curr(u32int id);
void tty16_set_bs_xmin(u8int xmin);
void tty16_xy(u32int need_x, u32int need_y);
void tty16_scroll(tty16_s *tty);
void tty16_put_char(u8int c);
void tty16_str(u8int *str);
void tty16_hex(u32int n);
void tty16_dec(u32int n);       //!!!max 0x0FFFFFFF 
void print16(u8int *text, ...);
void tty16_clear(void);
void tty16_remove(tty16_s *tty);
void tty16_move_cursor(u8int x, u8int y);
void tty16_set(u32int x0, u32int y0, u32int width, u32int height);
tty16_s *tty16_create(u32int x0, u32int y0, u32int width, u32int height);
tty16_s *tty16_get_curr(void);

#endif
