#ifndef MONITOR_H
#define MONITOR_H

#define COLOR_BACK      0
#define COLOR_FRONT     9

#include "common.h"

typedef struct {

    u8int   cur_x;
    u8int   cur_y;
    u16int  *vmem;

} vscreen_s;

void vmove_cursor(vscreen_s *vs);
void vscroll(vscreen_s *vs);
void monitor_char_put(char c);      //Write a single character out to the screen.
void vwrite_char(vscreen_s *vscrn, char c);
void monitor_clear();
void monitor_str_write(char *c);    // Output a null-terminated ASCII string to the monitor.
void wprint(u8int *text, ...);
void vwrite_str(vscreen_s *vscrn, char *c);
void write_hex(u32int n);
void write_dec(u32int n);
void vwrite_dec(vscreen_s *vscrn, u32int n);
void vprint(u8int *text, ...);
void set_video_vaddr(void* vaddr);
u16int *get_video_vaddr(void);
void del_vscreen(vscreen_s* vscrn);
vscreen_s* get_vscreen(void);

void *dump(void *addr, u32int count, u8int *type);

#endif
