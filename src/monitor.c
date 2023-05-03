#include "monitor.h"

u16int *video_memory = (u16int *)0xB8000;
u16int cursor_y;
u16int cursor_x;

 void move_cursor() {
    u16int curs_loc = cursor_y * 80 + cursor_x;

    outb(0x3D4, 14);
    outb(0x3D5, curs_loc >> 8);
    outb(0x3D4, 15);
    outb(0x3D5, curs_loc);
}

void vmove_cursor(vscreen_s *vs) {
    u16int curs_loc = vs->cur_y * 80 + vs->cur_x;

    outb(0x3D4, 14);
    outb(0x3D5, curs_loc >> 8);
    outb(0x3D4, 15);
    outb(0x3D5, curs_loc);
}

 void scroll() {
    u8int attr_color = (COLOR_BACK << 4 | COLOR_FRONT & 0x0F);
    u16int blank = 0x20 /*space*/ | (attr_color << 8);

    if(cursor_y >= 25) {

        for(int i = 0*80; i < 24*80; i++) {
            video_memory[i] = video_memory[i+80];
        }

        for(int i = 24*80; i < 25*80; i++) {
            video_memory[i] = blank;
        }

        cursor_y = 24;
    }
}

void vscroll(vscreen_s *vs) {
    u8int attr_color = (COLOR_BACK << 4 | COLOR_FRONT & 0x0F);
    u16int blank = 0x20 /*space*/ | (attr_color << 8);

    if(vs->cur_y >= 25) {

        for(int i = 0*80; i < 24*80; i++) {
            video_memory[i] = video_memory[i+80];
        }

        for(int i = 24*80; i < 25*80; i++) {
            video_memory[i] = blank;
        }

        vs->cur_y = 24;
    }
}

void monitor_char_put(char c) {
    u16int *location;
    u8int attr_color = (COLOR_BACK << 4 | COLOR_FRONT & 0x0F);
    u16int blank = attr_color << 8;

    if(c == 0x08 && cursor_x) {  //backspace
        cursor_x--;
        location = video_memory + (cursor_y*80 + cursor_x);
       *location = 0x20 | blank;
    }

    else if(c == 0x09) {        //TAB
        cursor_x = (cursor_x + 8) & ~(8-1);
    }

    else if(c == '\n') {
        cursor_x = 0;
        cursor_y++;
    }

    else if(c == '\r') {
        cursor_x = 0;
    }
    
    else if(c >= ' ') {
        location = video_memory + (cursor_y*80 + cursor_x);
       *location = c | blank;
        cursor_x++;
    }

    if(cursor_x >= 80) {
        cursor_x = 0;
        cursor_y++;
    }

    scroll();
    move_cursor();
}

void vwrite_char(vscreen_s *vscrn, char c) {

    u16int  *location;
    u8int   attr_color = (COLOR_BACK << 4 | COLOR_FRONT & 0x0F);
    u16int  blank = attr_color << 8;

    if(c == 0x08 && vscrn->cur_x) {

        vscrn->cur_x--;
        location = video_memory + (vscrn->cur_y*80 + vscrn->cur_x);
        *location = 0x20 | blank;
    }
    else if(c == 0x09) {

        vscrn->cur_x = (vscrn->cur_x + 8) &~(8-1);
    }
    else if(c == '\n') {

        vscrn->cur_x = 0;
        vscrn->cur_y++;
    }
    else if(c == '\r') {
        vscrn->cur_x = 0;
    }
    else if(c >= ' ') {

        location = video_memory + (vscrn->cur_y*80 + vscrn->cur_x);
        *location = c | blank;
        vscrn->cur_x++;
    }
    
    if(vscrn->cur_x > 80) {
        vscrn->cur_x = 0;
        vscrn->cur_y++;
    }

    vscroll(vscrn);
    vmove_cursor(vscrn);
}

void monitor_str_write(char *c) {

    for(int i = 0; c[i] != 0; i++) {
        monitor_char_put(c[i]);
    }
}

void vwrite_str(vscreen_s *vscrn, char *c) {

    for(int i = 0; c[i] != 0; i++) {
        vwrite_char(vscrn, c[i]);
    }
}

void monitor_clear() {

    u8int attr_color = (COLOR_BACK << 4 | COLOR_FRONT & 0x0F);
    u16int blank = 0x20/* space */ | (attr_color << 8);

    for(int i = 0; i < 25*80; i++) {
        video_memory[i] = blank;
    }

    cursor_x = 0;
    cursor_y = 0;
    move_cursor();
}

/*void write_hex (int numb) {
    const char mass[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    static char stri[11] = {'0', 'x', '0', '0', '0', '0', '0', '0', '0', '0', 0};
    char var;

    for(int i = 0; i < 8; i++) {
        var = (char)(numb & 0xFF);
        numb >> 4;
        var = mass[var];
        stri[9-i] = var;
    }
        monitor_str_write(stri);
    return;
} */

void write_hex(u32int n) {
    s32int tmp;
    char noZeroes = 1;

    monitor_str_write("0x");

    for(int i = 28; i > 0; i -= 4) {
        tmp = (n >> i) & 0xF;

        if(tmp == 0 && noZeroes != 0) {
            continue;
        }

        if (tmp >= 0xA) {
            noZeroes = 0;
            monitor_char_put (tmp-0xA+'a' );
        }
        else {
            noZeroes = 0;
            monitor_char_put( tmp+'0' );
        }
    }

    tmp = n & 0xF;
    if (tmp >= 0xA) {
         monitor_char_put (tmp-0xA+'a');
    }
    else {
        monitor_char_put (tmp+'0');
    }
    return;
}

void write_dec(u32int n) {

    if (n == 0) {
        monitor_char_put('0');
        return;
    }

    s32int acc = n;
    char c[32];
    int i = 0;

    while (acc > 0) {
        c[i] = '0' + acc%10;
        acc /= 10;
        i++;
    }
    c[i] = 0;

    char c2[32];
    c2[i--] = 0;
    int j = 0;

    while(i >= 0) {
         c2[i--] = c[j++];
    }

    monitor_str_write(c2);
    return;
}

void set_video_vaddr(void* vaddr) {

    video_memory = (u16int*) vaddr;
    return;
}

u16int *get_video_vaddr(void) {

    return video_memory;
}

vscreen_s* get_vscreen(void) {

    vscreen_s* tmp = (vscreen_s*)umalloc(sizeof(vscreen_s));
    memset(tmp, 0, sizeof(vscreen_s));

    return tmp;
}

vscreen_s* kget_vscreen(void) {

    vscreen_s* tmp = (vscreen_s*)kmalloc(sizeof(vscreen_s));
    memset(tmp, 0, sizeof(vscreen_s));
    return tmp;
}

void del_vscreen(vscreen_s* vscrn) {

    kfree(vscrn);
    return;
}

void vwrite_dec(vscreen_s *vscrn, u32int n) {

    if (n == 0) {
        vwrite_char(vscrn, '0');
        return;
    }

    s32int acc = n;
    char c[32];
    int i = 0;

    while (acc > 0) {
        c[i] = '0' + acc%10;
        acc /= 10;
        i++;
    }
    c[i] = 0;

    char c2[32];
    c2[i--] = 0;
    int j = 0;

    while(i >= 0) {
        c2[i--] = c[j++];
    }
    
    vwrite_str(vscrn, c2);
    return;
}

void vwrite_hex(vscreen_s *vscrn, u32int n) {
    s32int tmp;
    char noZeroes = 1;

    vwrite_str(vscrn, "0x");

    for(int i = 28; i > 0; i -= 4) {
        tmp = (n >> i) & 0xF;

        if(tmp == 0 && noZeroes != 0) {
            continue;
        }

        if (tmp >= 0xA) {
            noZeroes = 0;
            vwrite_char(vscrn, tmp-0xA+'a');
        }
        else {
            noZeroes = 0;
            vwrite_char(vscrn, tmp+'0');
        }
    }

    tmp = n & 0xF;
    if (tmp >= 0xA) {
        vwrite_char(vscrn, tmp-0xA+'a');
    }
    else {
        vwrite_char(vscrn, tmp+'0');
    }
    return;
}

void wprint(u8int *text, ...) {

    u32int *args = (u32int *)&text;
    args++;

    for(u32int i = 0; text[i] != 0; i++) {

        if(text[i] == '%') {
            i++;

            if(text[i] == 'x') {
                write_hex(*args);
                args++;
            }
            else if(text[i] == 'd') {
                write_dec(*args);
                args++;
            }
            else if(text[i] == 's') {
                monitor_str_write((s8int *)*args);
                args++;
            }
            else {

            }
        }
        else {
            monitor_char_put(text[i]);
        }
    }
    return;
}

void vprint(u8int *text, ...) {

    vscreen_s *vs = curr_thread->tty;
    u32int *args = (u32int *)&text;
    args++;

    for(u32int i = 0; text[i] != 0; i++) {

        if(text[i] == '%') {
            i++;

            if(text[i] == 'x') {
                vwrite_hex(vs, *args);
                args++;
            }
            else if(text[i] == 'd') {
                vwrite_dec(vs, *args);
                args++;
            }
            else if(text[i] == 's') {
                vwrite_str(vs, (s8int *)*args);
                args++;
            }
            else {

            }
        }
        else {
            vwrite_char(vs, text[i]);
        }
    }
    return;
}

void *dump(void *addr, u32int count, u8int *type) {

    u8int *dec = "dec";
    u8int *hex = "hex";
    u8int *n = addr;

    if(!strcmp(type, hex)) {

        u32int c = 0;
        u32int i = 0;
        u32int tmp;
        char noZeroes = 1;
        u8int *ret = kmalloc(count + (count / 3) + 2);

        for(int j = 0; j < count; j++) {
            tmp = n[j] & 0xF;

            if(c == 2) {
                c = 0;
                ret[i] = ' ';
                i++;
            }
            c++;

            if(tmp == 0 && noZeroes != 0) {
                ret[i] = '0';
                i++;
                continue;
            }

            if (tmp >= 0xA) {
                noZeroes = 0;
                ret[i] = (tmp - 0xA) + 'A';
                i++;
            }
            else {
                noZeroes = 0;
                ret[i] = tmp + '0';
                i++;
            }
        }
        ret[i] = '\0';
        return ret;
    }
}
