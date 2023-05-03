
#ifndef     WINDOWS_H
#define     WINDOWS_H

#include "common.h"
#include "izo.h"
#include "help_func.h"
#include "bitmap.h"
#include "list.h"
#include "vfs.h"

#define     VESA_BLACK      0x00000000
#define     VESA_BLUE       0x000000FF
#define     VESA_GREEN      0x0000FF00
#define     VESA_RED        0x00FF0000
#define     VESA_MAGENTA    0x00FF00FF
#define     VESA_WHITE      0x00FFFFFF
#define     VESA_UBUNT_TERM 0xFF300A24

#define     WINDOW_SUPER     0x00
#define     WINDOW_NORMAL    0x01
#define     WINDOW_CONTROL   0x02
#define     WINDOW_ALERT     0x03
#define     WINDOW_DESKT_BAR 0x04

#define     TITLE_BAR_HEIGHT 0x17
#define     WINMSG_KEYB      0x01

typedef struct window window_s;
typedef struct winmsg winmsg_s;
typedef struct wind_dirty wind_dirty_s;
typedef struct point point_s;

struct window {

    u8int       name[32];
    s32int      x;
    s32int      y;

    s32int      width;
    s32int      height;
    s32int      orig_width;
    s32int      orig_height;

    u32int      fill_color;
    u32int      border_color;

    window_s    *parent;
    s32int      is_focus;
    s32int      is_minimized;
    s32int      is_maximized;
    s32int      depth;
    s32int      type;

    u32int      *frame_buff;
    u32int      *blend_buff;

    list_s      *under_windows;
    list_s      *above_windows;

    rect_s      intersect_rect;
    gtreenode_s *self;

};

struct winmsg {

    s32int      curs_x;
    s32int      curs_y;
    s32int      change_x;
    s32int      change_y;
    s32int      type;
    s32int      sub_type;

    u8int       keyb_key;
    window_s    *window;   
};

struct wind_dirty {

    window_s *window;
    s32int   len;
    rect_s   rects[3];
};

struct point {

    s32int x;
    s32int y;
};

void windows_init(void);
void close_button_init(void);
void minimize_button_init(void);
void maximize_button_init(void);

window_s *window_create(window_s *parent, s32int x, s32int y, s32int width, s32int height, s32int type, u8int *name);
void recur_add_under_windows(window_s *window, gtreenode_s *subroot);
void window_title_bar(window_s *window);
void window_close_button(window_s *window);
void window_minimize_button(window_s *window);
void window_maximize_button(window_s *window);

void window_display(window_s *wind, rect_s *rects, s32int size);
void display_recur(gtreenode_s *parent);
void media_mem_upd(rect_s *rects, s32int count);

point_s get_dev_coordns(window_s *wind);
u32int get_wind_pixel(window_s *wind, s32int x, s32int y);
s32int is_windows_overlap(window_s *window1, window_s *window2);
window_s *get_super_window(void);
void window_round_corner(window_s *wind);

void intersections_calc(rect_s *rect, window_s **array, u32int *size);
void sort_wind_array(window_s **array, u32int size);
void blend_wind_rect(window_s *top_w, window_s *und_w);
void blend_windows(window_s *wind);


#endif