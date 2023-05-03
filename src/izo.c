#include "izo.h"

u32int fill_color;
u32int *screen = NULL;
s32int screen_width;
s32int screen_height;

u32int font_color = 0x00FFFF00;

canvas_s canvas_create(s32int width, s32int height, u32int *buff) {

    canvas_s canvas;
    canvas.width = width;
    canvas.height = height;
    canvas.buff = buff;

    return canvas;
}

rect_s rect_create(s32int x, s32int y, s32int width, s32int height) {

    rect_s rect;
    rect.width = width;
    rect.height = height;
    rect.x = x;
    rect.y = y;

    return rect;
}

region_rect_s region_create(rect_s rect, u32int *region) {

    region_rect_s reg_rect;
    reg_rect.rect = rect;
    reg_rect.region = region;

    return reg_rect;
}

s32int get_pixel(canvas_s *canvas, s32int x, s32int y) {

    s32int ret = canvas->width * y + x;
    return ret;
}

void set_pixel(canvas_s *canvas, u32int val, s32int x, s32int y) {

    s32int pixel = get_pixel(canvas, x, y);
    canvas->buff[pixel] = val;
    return;
}

void set_fillcolor(u32int color) {

    fill_color = color;
    return;
}

void draw_rect(canvas_s *canvas, s32int st_x, s32int st_y, s32int width, s32int height) {

    for(s32int y = st_y; y < st_y + height; y++) {
        for(s32int x = st_x; x < st_x + width; x++) {
            set_pixel(canvas, fill_color, x, y);
        }
    }
    return;
}

void draw_rect_region(canvas_s *canvas, region_rect_s *region) {

    s32int pix;

    for(s32int y = region->rect.y; y < region->rect.y + region->rect.height; y++) {
        for(s32int x = region->rect.x; x < region->rect.x + region->rect.width; x++) {

            pix = region->rect.width * (y - region->rect.y) + (x - region->rect.x);
            if(region->region[pix] != 0x00) {
                set_pixel(canvas, region->region[pix], x, y);
            }
        }
    }
    return;
}

void paint_clip_rect(canvas_s *canvas, region_rect_s *region, s32int crop_width) {

    s32int pix;

    for(s32int y = region->rect.y; y < region->rect.y + region->rect.height; y++) {
        for(s32int x = region->rect.x; x < region->rect.x + region->rect.width; x++) {

            pix = crop_width * (y - region->rect.y) + (x - region->rect.x);
            if(region->region[pix] != 0) {
                set_pixel(canvas, region->region[pix], x, y);
            }
        }

    }
    return;
}

void paint2_clip_rect(canvas_s *canvs, region_rect_s *region, s32int crop_width, s32int crop_x, s32int crop_y) {

    s32int pix;

    for(s32int y = region->rect.y; y < region->rect.y + region->rect.height; y++) {
        for(s32int x = region->rect.x; x < region->rect.x + region->rect.width; x++) {

            pix = crop_width * (y - crop_y) + (x - crop_x);
            if(region->region[pix] != 0) {
                set_pixel(canvs, region->region[pix], x, y);
            }
        }
    }
    return;
}

void remov_sharp_edges(canvas_s *canvas, s32int st_x, s32int st_y, s32int direct, s32int pixels, u32int alpha_end, u32int alpha_mid) {

    u32int pix;
    u32int color;
    u32int alpha;
    u32int val;

    u32int count = pixels;
    s32int x = st_x;
    s32int y = st_y;

    while(count-- > 0) {

        pix = get_pixel(canvas, x, y);
        color = canvas->buff[pix];

        if(count == 0 || count == pixels - 1) {
            alpha = alpha_end;
        }
        else {
            alpha = alpha_mid;
        }

        val = ((color << 8) >> 8) | ((alpha << 24) & 0xFF000000);
        set_pixel(canvas, val, x, y);
        y++;
        x += direct;
    }
    return;
}

void round_corner_effect(canvas_s *canvas) {

    for(s32int y = 0; y < 3; y++) {

        for(s32int x = 0; x < 3 - y; x++) {

            set_pixel(canvas, 0x00, x, y);
            set_pixel(canvas, 0x00, canvas->width - 3 + y + x, y);
        }
    }
    remov_sharp_edges(canvas, 0, canvas->width - 4, 1, 4, 0x88, 0xEE);
    remov_sharp_edges(canvas, 0, canvas->width - 5, 1, 5, 0xFF, 0xEE);
    remov_sharp_edges(canvas, 1, canvas->width - 5, 1, 4, 0xFF, 0xFF);

    remov_sharp_edges(canvas, 0, 3, -1, 4, 0x88, 0xEE);
    remov_sharp_edges(canvas, 0, 4, -1, 5, 0x88, 0xEE);
    remov_sharp_edges(canvas, 1, 4, -1, 4, 0x88, 0xEE);

    return;
}

void pnt_line(canvas_s *canvs, s32int x0, s32int y0, s32int x1, s32int y1) {

    s32int  px = x0;
    s32int  py = y0;
    s32int  dx = x1 - x0;  
    s32int  dy = y1 - y0;
    s32int  abs_dx = abs(dx);
    s32int  abs_dy = abs(dy);
    s32int  sign_dx = sign(dx);
    s32int  sign_dy = sign(dy);
    s32int  x_tmp = 0;
    s32int  y_tmp = 0;

    set_pixel(canvs, fill_color, px, py);

    if(abs_dx >= abs_dy) {

        for(s32int i = 0; i < abs_dx; i++) {
            y_tmp += abs_dy;

            if( y_tmp >= abs_dx) {

                y_tmp -= abs_dx;
                py += sign_dy;
            }
            px += sign_dx;
            set_pixel(canvs, fill_color, px, py);
        }
    }
    else {
        for(s32int i = 0; i < abs_dy; i++) {
            x_tmp += abs_dx;

            if(x_tmp >= abs_dy) {

                x_tmp -= abs_dy;
                px += sign_dx;
            }
            py += sign_dy;
            set_pixel(canvs, fill_color, px, py);
        }
    }
    return;
}

s32int is_line_overlap(s32int line1_x1, s32int line1_x2, s32int line2_x1, s32int line2_x2){

    s32int ret1 = (line1_x1 >= line2_x1 && line1_x1 <= line2_x2) && (line1_x2 >= line2_x1 && line1_x2 <= line2_x2);
    s32int ret2 = (line1_x1 >= line2_x1 && line1_x1 <= line2_x2) && (line1_x2 > line2_x2);
    s32int ret3 = line1_x1 < line2_x1 && (line1_x1 >= line2_x1 && line1_x2 <= line2_x2);
    s32int ret4 = (line1_x1 < line2_x1) && (line1_x1 > line2_x2);

    return ret1 || ret2 || ret3 || ret4;
}

s32int is_rect_overlap(rect_s rect0, rect_s rect1) {

    if((rect0.x > rect1.x + rect1.width) || (rect1.x > rect0.x + rect0.width)) {
        return 0;
    }
    if((rect0.y > rect1.y + rect1.height) || (rect1.y > rect0.y + rect0.height)) {
        return 0;
    }
    return 1;
}

s32int is_point_in_rect(s32int x, s32int y, rect_s *rect) {

    return (x >= rect->x && x < rect->x + rect->width) && (y >= rect->y && y < rect->y + rect->width);
}

rect_s find_rect_overlab(rect_s r0, rect_s r1) {

    rect_s ret;
    s32int mx = max(r0.x, r1.x);
    s32int my = max(r0.y, r1.y);

    if(r0.x + r0.width > r1.x + r1.width) {
        ret.width = r1.x + r1.width - mx;
    }
    else {
        ret.width = r0.x + r0.width - mx;
    }
    ret.x = mx;

    if(r0.y + r0.height > r1.y + r1.height) {
        ret.height = r1.y + r1.height - my;
    }
    else {
        ret.height = r0.y + r0.height - my;
    }
    ret.y = my;

    return ret;
}

void draw_test() {

    u32int *buff = vesa_mem;
    fill_color = 0x00FF0000;
    s32int *reg = kmalloc(640 * 360 * 4);

    for(u32int i = 0; i < 1280 * 720; i++) {
        reg[i] = 0x00FFFF00;
    }

    canvas_s canvas = canvas_create(1280, 720, buff);
    rect_s rect = rect_create(0, 0, 200, 100);
    //draw_rect(&canvas, 600, 45, rect.width, rect.height);
    //draw_rect(&canvas, 45, 250, rect.width, rect.height);

    region_rect_s *region = kmalloc(sizeof(region_rect_s));
    rect_s gr_rect = rect_create(45, 400, 200, 100);
    region->region = reg;
    region->rect = gr_rect;
    //draw_rect_region(&canvas, region);
    //paint_clip_rect(&canvas, region, 100);
    //paint2_clip_rect(&canvas, region, 10, 10, 10);
    pnt_line(&canvas, 50, 600, 50, 700);
    pnt_line(&canvas, 50, 600, 500, 700);
    pnt_line(&canvas, 500, 700, 50, 700);

    cursor_s cur;
    cur = pnt_text(&canvas, 0, 400, " Hello   , world!\nHi PARF\n");
    cur = pnt_text(&canvas, cur.x, cur.y, "I am here\n");
    cur = pnt_text(&canvas, cur.x, cur.y, " HEX:(");
    cur = pnt_hex(&canvas, cur.x, cur.y, (u32int)0x89ABCDEF);
    cur = pnt_text(&canvas, cur.x, cur.y, ") DEC:(");
    cur = pnt_dec(&canvas, cur.x, cur.y, 0xFFFFFFF0);
    cur = pnt_text(&canvas, cur.x, cur.y, ")\n");
    cur = pprint(&canvas, cur.x, cur.y, "a = %x + %d / %x - 1%d = %sHi ;)\n", 0xFF, 345, 0x0A, 5, "hyu znaet\n");



    kfree(reg);
    kfree(region);
    return;
}

u32int get_font_color(void) {
    return font_color;
}

void set_font_color(u32int color) {

    font_color = color | 0xFF000000;
    return;
}

cursor_s pnt_text(canvas_s *canvs, s32int st_x, s32int st_y, u8int *text) {

    u32int col = st_x;
    u32int row = st_y;
    u32int step = 8;
    u32int stop = 0;

    s32int len = strlen(text);
    u8int *font = get_font1_array();
    u8int chr = 0;
    cursor_s cur;

    for(u32int i = 0; i < len; i++) {

        if(text[i] == '\0') {
            chr = 0;
            stop = 1;
        }
        else if(text[i] == '\r') {
            chr = 0;
            col = 0;
        }
        else if(text[i] == '\n') {
            col = 0;
            row += step * 2;
            chr = 0;
        }
        else if(text[i] == '\t') {
            col = col + step * 4 - col % 4;
            chr = 0;
        }
        else {
            col += step;
            chr = text[i];
        }

        cur.x = col;
        cur.y = row;

        if(stop) {
            break;
        }

        if(chr != 0 && canvs->width != 0) {
            for(u32int y = 0; y < FONT_HEIGHT; y++) {
                for(u32int x = 0; x < FONT_WIDTH; x++) {

                    if(font[text[i] * FONT_HEIGHT + y] >> (7 - x) & 0x01) {
                        canvs->buff[(canvs->width * (row + y)) + (col + x)] = font_color;
                    }
                }
            }
        }
        //col += 6;
    }
    return cur;
}

cursor_s pnt_hex(canvas_s *canvs, s32int x, s32int y, u32int val) {

    s32int tmp;
    u8int noZero = 1;

    cursor_s cur = pnt_text(canvs, x, y, "0x");

    for(s32int i = 28; i > 0; i -= 4) {

        tmp = (val >> i) & 0x0F;

        if(tmp == 0 && noZero != 0) {
            continue;
        }

        if(tmp >= 0x0A) {
            noZero = 0;
            tmp = tmp - 0x0A + 'A';
         
            cur = pnt_text(canvs, cur.x, cur.y, (u8int *)&tmp);
        }
        else {
            noZero = 0;
            tmp = tmp + '0';
            cur = pnt_text(canvs, cur.x, cur.y, (u8int *)&tmp);
        }
    }
    tmp = val & 0x0F;

    if(tmp >= 0x0A) {
        tmp = tmp - 0x0A + 'A';
        cur = pnt_text(canvs, cur.x, cur.y, (u8int *)&tmp);
    }
    else {
        tmp = tmp + '0';
        cur = pnt_text(canvs, cur.x, cur.y, (u8int *)&tmp);
    }

    return cur;
}

cursor_s pnt_dec(canvas_s *canvs, s32int x, s32int y, u32int val) {

    cursor_s cur;

    if(val > 0x0FFFFFFF) {
        cur = pnt_text(canvs, x, y, "pnt_dec(): ERROR\n");
        return cur;
    }

    if(val == 0) {
        cur = pnt_text(canvs, x, y, "0");
        return cur;
    }
    
    s32int acc = val;
    s32int i = 0;
    u8int c[32];
    
    while(acc > 0) {

        c[i] = '0' + acc % 10;
        acc /= 10;
        i++;
    }
    c[i] = 0;

    s32int j = 0;
    u8int c2[32];
    c2[i--] = 0;

    while(i >= 0) {
        c2[i--] = c[j++];
    }

    cur = pnt_text(canvs, x, y, c2);

    return cur;
}

cursor_s pprint(canvas_s *canvs, s32int x, s32int y, u8int *text, ...) {

    cursor_s cur;
    cur.x = x;
    cur.y = y;

    u32int *args = (u32int *)&text;
    args++;

    u8int tmp[2] = {0};

    for(u32int i = 0; text[i] != 0; i++) {

        if(text[i] == '%') {
            i++;

            if(text[i] == 's') {
                cur = pnt_text(canvs, cur.x, cur.y, (u8int *)*args);
                args++;
            }
            else if(text[i] == 'x') {
                cur = pnt_hex(canvs, cur.x, cur.y, *args);
                args++;
            }
            else if(text[i] == 'd') {
                cur = pnt_dec(canvs, cur.x, cur.y, *args);
                args++;
            }
            else {
                
            }
        }
        else {
            tmp[0] = text[i];
            cur = pnt_text(canvs, cur.x, cur.y, tmp);
        }
    }
    return cur;
}

u32int blend_colors(u32int color1, u32int color2) {

    u32int alp1 = (color1 >> 24) & 0x000000FF;
    u32int red1 = (color1 >> 16) & 0x000000FF;
    u32int green1 = (color1 >> 8) & 0x000000FF;
    u32int blue1 = color1 & 0x000000FF;
    u32int t1 = alp1 * 1.0 / 255;

    u32int alp2 = (color2 >> 24) & 0x000000FF;
    u32int red2 = (color2 >> 16) & 0x000000FF;
    u32int green2 = (color2 >> 8) & 0x000000FF;
    u32int blue2 = color2 & 0x000000FF;
    u32int t2 = alp2 * 1.0 / 255;

    u32int r1 = (u32int)(t1 * red1);
    u32int g1 = (u32int)(t1 * green1);
    u32int b1 = (u32int)(t1 * blue1);

    u32int r2 = (u32int)(t2 * red2);
    u32int g2 = (u32int)(t2 * green2);
    u32int b2 = (u32int)(t2 * blue2);

    u32int t3 = (255 - alp1) * 1.0 / 255;
    r1 = r1 + t3 * r2;
    g1 = g1 + t3 * g2;
    b1 = b1 + t3 * b2;

    u32int new_alp = (u32int)(alp1 + t3 * alp2);
    u32int col_over_col = (new_alp << 24) | (r1 << 16) | (g1 << 8) |  (b1 << 0);

    return col_over_col;
}

/*
u32int blend_colors(u32int color1, u32int color2) {
   u32int alpha1 = GET_ALPHA(color1);
   u32int red1 = GET_RED(color1);
   u32int green1 = GET_GREEN(color1);
   u32int blue1 = GET_BLUE(color1);

   u32int alpha2 = GET_ALPHA(color2);
   u32int red2 = GET_RED(color2);
   u32int green2 = GET_GREEN(color2);
   u32int blue2 = GET_BLUE(color2);

   u32int r = (u32int)((alpha1 * 1.0 / 255) * red1);
   u32int g = (u32int)((alpha1 * 1.0 / 255) * green1);
   u32int b = (u32int)((alpha1 * 1.0 / 255) * blue1);

    r = r + (((255 - alpha1) * 1.0 / 255) * (alpha2 * 1.0 / 255)) * red2;
    g = g + (((255 - alpha1) * 1.0 / 255) * (alpha2 * 1.0 / 255)) * green2;
    b = b + (((255 - alpha1) * 1.0 / 255) * (alpha2 * 1.0 / 255)) * blue2;

   u32int new_alpha = (u32int)(alpha1 + ((255 - alpha1) * 1.0 / 255) * alpha2);
    u32int color1_over_color2 = (new_alpha << 24) |  (r << 16) | (g << 8) | (b << 0);
    return color1_over_color2;
}
*/