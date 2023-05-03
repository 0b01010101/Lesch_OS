
#include    "tty16.h"

list_s  tty16_list;
u32int  tty16_id_next = 0;
tty16_s *curr_tty16;
tty16_s *new_tty16;

tty16_s *tty16_init(void) {

    list_init(&tty16_list);
    tty16_s * tty = tty16_create(0, 0, SCRN_WIDTH, SCRN_HEIGHT);
    get_curr_proc()->tty16 = tty;
    return tty;
} 

tty16_s *tty16_create(u32int x0, u32int y0, u32int width, u32int height) {

    tty16_s *tmp = (tty16_s *)kmalloc(sizeof(tty16_s));
    if(!tmp) {
        wprint("ERR: tty create!\n");
        return NULL;
    }
    tmp->list_item.list = NULL;
    tmp->id = tty16_id_next;
    tmp->x0 = x0;
    tmp->y0 = y0;
    tmp->xmax = width;
    tmp->ymax = height;
    tmp->cur_x = 0;
    tmp->cur_y = 0;
    tmp->bs_xmin = 0;
    tmp->width = SCRN_WIDTH;
    //tmp->height = SCRN_HEIGHT - 2 * tmp->y0;
    tmp->col_back = COL_BLACK;
    tmp->col_txt = COL_BLUE;
    tmp->vbuff = (u16int *)ucalloc(sizeof(u16int) * SCRN_WIDTH * SCRN_HEIGHT, 1);

    if(!tmp->vbuff) {
        wprint("ERR: tty16 alloc buff\n");
        return NULL;
    }
    if(tty16_list.count == 0) {
        curr_tty16 = tmp;
    }
    list_add(&tty16_list, &tmp->list_item);
    tty16_id_next++;

    return tmp;
}

void tty16_remove(tty16_s *tty) {

    ufree(tty->vbuff);
    list_remove(&tty->list_item);
    kfree(tty);
    
    return;
}

void tty16_set_curr(u32int id) {

    tty16_s *tmp = (tty16_s *)tty16_list.first;
    u32int idx = tty16_list.count;

    do {
        if(tmp->id == id) {
            curr_tty16 = tmp;
            return;
        }
        tmp = (tty16_s *)tmp->list_item.next;
        idx--;

    } while(idx);

    return;
}

tty16_s *tty16_get_curr(void) {

    return curr_tty16;
}

void tty16_swap_buff(void) {

    u16int *strt;
    u16int *vm = (u16int *)0xB8000;     //get_video_vaddr();
    u16int *sap_buff = curr_tty16->vbuff;
    s8int sap_y = curr_tty16->ymax;

    u32int i = curr_tty16->y0;
    while(sap_y >= 0) {
        
        strt = vm + ((i * SCRN_WIDTH) + curr_tty16->x0);
        memcpy(strt, sap_buff, sizeof(u16int) * curr_tty16->xmax);
        sap_buff += curr_tty16->xmax;
        sap_y--;
        i++;
    }
    return;
}

void tty16_set(u32int x0, u32int y0, u32int width, u32int height) {

    curr_tty16->x0 = (u8int)x0;
    curr_tty16->y0 = (u8int)y0;
    curr_tty16->xmax = (u8int)width;
    curr_tty16->ymax = (u8int)height;

    return;
}

void tty16_xy(u32int need_x, u32int need_y) {

    curr_tty16->cur_x = need_x;
    curr_tty16->cur_y = need_y;
    return;
}

void tty16_set_bs_xmin(u8int xmin) {

    curr_tty16->bs_xmin = xmin;
    return;
} 

void tty16_put_char(u8int c) {

    tty16_s *tty = get_curr_proc()->tty16;
    u8int attr_b = (tty->col_back << 4) | (tty->col_txt & 0x0F);
    u16int blank = attr_b << 8;
    u16int *loc;

    if(tty->cur_x > tty->xmax) {

        tty->cur_x = 0;
        tty->cur_y++;
    }
    if(tty->cur_x < 0) {

        loc = tty->vbuff + tty->cur_y * tty->xmax;
        *loc = 0x20 | blank;

        tty->cur_y--;
        tty->cur_x = tty->xmax;
    }
    
    if((c == 0x08) && (tty->cur_x != 0)) {

        tty->cur_x--;
        if(tty->cur_x < tty->bs_xmin) {
            tty->cur_x = tty->bs_xmin;
        }
        loc = tty->vbuff + (tty->cur_y * tty->xmax + tty->cur_x);
        *loc = 0x20 | blank;
    }
    else if(c == '\n') {

        tty->cur_x = 0;
        tty->cur_y++;
    }
    else if(c == 0x09) {

        tty->cur_x = (tty->cur_x + 8) & ~(8 - 1);
    }
    else if(c == '\r') {

        tty->cur_x = 0;
    }
    else {

        loc = tty->vbuff + (tty->cur_y * tty->xmax + tty->cur_x);
        *loc = c | blank;
        tty->cur_x++;
    }
    tty16_scroll(tty);
    tty16_move_cursor(tty->x0 + tty->cur_x, tty->y0 + tty->cur_y);

    return;
}

void tty16_scroll(tty16_s *tty) {

    u8int attr_b = (tty->col_back << 4) | (tty->col_txt & 0x0F);

    if(tty->cur_y >= tty->ymax) {

        u32int i;

        for(i = 0; i < (tty->ymax - 1) * tty->xmax; i++) {

            tty->vbuff[i] = tty->vbuff[i+tty->xmax];
        }

        for(i = (tty->ymax - 1) * tty->xmax; i < tty->ymax * tty->xmax; i++) {

            tty->vbuff[i] = 0x20 | attr_b;
        }

        tty->cur_y = tty->ymax - 1;
    }
    return;
}

void tty16_move_cursor(u8int x, u8int y) {

    //wprint("move cursor...");
    u16int curs_loc = y * SCRN_WIDTH + x;

    outb(0x03D4, 14);
    outb(0x03D5, curs_loc >> 8);
    outb(0x03D4, 15);
    outb(0x03D5, curs_loc);

    return;
}

void tty16_clear(void) {

    u8int attr_b = (curr_tty16->col_back << 4) | (curr_tty16->col_txt & 0x0F);
    u16int blank = 0x20 | (attr_b << 8);

    for(u32int i = 0; i < curr_tty16->xmax * curr_tty16->ymax; i++) {

        curr_tty16->vbuff[i] = blank;
    }
    tty16_move_cursor(curr_tty16->x0, curr_tty16->y0);

    return;
}

void tty16_str(u8int *str) {

    u32int i = 0;

    while(str[i]) {
        tty16_put_char(str[i++]);
    }
    return;
}

void tty16_hex(u32int n) {

    s32int tmp;
    u8int noZer = 1;

    tty16_str("0x");

    for(u32int i = 28; i > 0; i -= 4) {
        tmp = (n >> i) & 0x0F;

        if(tmp == 0 && noZer != 0) {
            continue;
        }
        if(tmp >= 0x0A) {
            noZer = 0;
            tty16_put_char(tmp - 0x0A + 'A');
        }
        else {
            noZer = 0;
            tty16_put_char(tmp + '0');
        }
    }
    tmp = n & 0x0F;

    if(tmp >= 0x0A) {
        tty16_put_char(tmp - 0x0A + 'A');
    }
    else {
        tty16_put_char(tmp + '0');
    }
    return;
}

void tty16_dec(u32int n) {          //!!!max 0x0FFFFFFF

    if(n == 0) {
        tty16_put_char('0');
        return;
    }
    s32int ac = n;
    s32int i = 0;
    u8int c[32];

    while(ac > 0) {

        c[i] = '0' + ac%10;
        ac /= 10;
        i++;
    }
    c[i] = 0;

    u32int j = 0;
    u8int cc[32];
    cc[i--] = 0;

    while(i >= 0) {
        cc[i--] = c[j++];
    }
    tty16_str(cc);

    return;
}

void print16(u8int *txt, ...) {

    u32int *args = (u32int *)&txt;
    args++;

    for(u32int i = 0; txt[i] != 0; i++) {

        if(txt[i] == '%') {
            i++;

            if(txt[i] == 'x') {
                tty16_hex(*args);
                args++;
            }
            else if(txt[i] == 'd') {
                tty16_dec(*args);
                args++;
            }
            else if(txt[i] == 's') {
                tty16_str((u8int *)*args);
                args++;
            }
            else {

            }
        }
        else {
            tty16_put_char(txt[i]);
        }
    }
    return;
}
