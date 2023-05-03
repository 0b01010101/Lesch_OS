#include "windows.h"

cursor_s cur;
canvas_s cnvs;

canvas_s canvas;
gtree_s  *tree_windows;
list_s   *list_windows;

window_s *desktop_bar;
window_s *focus_window;
window_s *windows_array[100];

u32int *screen;
u32int *media_buff;

s32int screen_width;
s32int screen_height;

bitmap_s *norm_close_bmp;
bitmap_s *norm_minimize_bmp;
bitmap_s *norm_maximize_bmp;

bitmap_s *hl_close_bmp;
bitmap_s *hl_minimize_bmp;
bitmap_s *hl_maximize_bmp;

region_rect_s close_reg;
region_rect_s minimize_reg;
region_rect_s maximize_reg;

region_rect_s hl_close_reg;
region_rect_s hl_minimize_reg;
region_rect_s hl_maximize_reg;

u32int colors_title_bar[TITLE_BAR_HEIGHT] = {0xff59584f, 0xff5f5d53, 0xff58564e, 0xff57554d, 0xff56544c, \
    0xff55534b, 0xff54524a, 0xff525049, 0xff514f48, 0xff504e47, 0xff4e4c45, 0xff4e4c45, \
    0xff4c4a44, 0xff4b4943, 0xff4a4842, 0xff484741, 0xff46453f, 0xff45443f, \
    0xff44433e, 0xff43423d, 0xff42413c, 0xff403f3a, 0xff3f3e3a};

void close_button_init(void) {

    norm_close_bmp = bmp_create("/close.bmp");
    hl_close_bmp = bmp_create("/hl_close.bmp");
    close_reg.rect.x = 7;
    close_reg.rect.y = 3;
    close_reg.rect.width = norm_close_bmp->width;
    close_reg.rect.height = norm_close_bmp->height;
    close_reg.region = (void *)norm_close_bmp->image_bytes;

    hl_close_reg.rect.x = 7;
    hl_close_reg.rect.y = 3;
    hl_close_reg.rect.width = hl_close_bmp->width;
    hl_close_reg.rect.height = hl_close_bmp->height;
    hl_close_reg.region = (void *)hl_close_bmp->image_bytes;
    return;
}

void minimize_button_init(void) {

    norm_minimize_bmp = bmp_create("/minimize.bmp");
    hl_minimize_bmp = bmp_create("/hl_minimize.bmp");
    minimize_reg.rect.x = 7 + 17 + 2;
    minimize_reg.rect.y = 3;
    minimize_reg.rect.width = norm_minimize_bmp->width;
    minimize_reg.rect.height = norm_minimize_bmp->height;
    minimize_reg.region = (void *)norm_minimize_bmp->image_bytes;

    hl_minimize_reg.rect.x = 7 + 17 + 2;
    hl_minimize_reg.rect.y = 3;
    hl_minimize_reg.rect.width = hl_minimize_bmp->width;
    hl_minimize_reg.rect.height = hl_minimize_bmp->height;
    hl_minimize_reg.region = (void *)hl_minimize_bmp->image_bytes;

    return;
}

void maximize_button_init(void) {

    norm_maximize_bmp = bmp_create("/maximize.bmp");
    hl_minimize_bmp = bmp_create("/hl_maximize.bmp");
    maximize_reg.rect.x = 7 + 17 + 2 + 17 + 2;
    maximize_reg.rect.y = 3;
    maximize_reg.rect.width = norm_maximize_bmp->width;
    maximize_reg.rect.height = norm_maximize_bmp->height;
    maximize_reg.region = (void *)norm_maximize_bmp->image_bytes;

    hl_maximize_reg.rect.x = 7 + 17 + 2 + 17 + 2;
    hl_maximize_reg.rect.y = 3;
    hl_maximize_reg.rect.width = hl_maximize_bmp->width;
    hl_maximize_reg.rect.height = hl_maximize_bmp->height;
    hl_maximize_reg.region = (void *)hl_maximize_bmp->image_bytes;

    return;
}

void windows_init(void) {

    screen = vesa_mem;
    screen_width = vesa_get_resolX();
    screen_height = vesa_get_resolY();
    media_buff = kmalloc(screen_width * screen_height * 4);
    canvas = canvas_create(screen_width, screen_height, media_buff);

    list_windows = list_create();
    tree_windows = tree_create();

    window_s *wind = window_create(NULL, 0, 0, screen_width, screen_height, WINDOW_SUPER, "desktop");
    bitmap_s *wallpaper = bmp_create("/tian.bmp");
    //bmp_display(wallpaper);
    bmp_to_framebuff(wallpaper, wind->frame_buff);
    //blend_windows(wind);      //->blend_wind_rect->blend_colors  FPU exeption !!! need correct
    memcpy(screen, wind->frame_buff, screen_width * screen_height * 4);

    close_button_init();
    minimize_button_init();
    maximize_button_init();

    return;
}

void media_mem_upd(rect_s *rects, s32int count) {

    if(!rects) {
        for(u32int i = 0; i < screen_width * screen_height; i++) {
            screen[i] = media_buff[i];
        }
    }
    else {
        for( u32int i = 0; i < count; i++) {
            rect_s rect = rects[i];
            for(s32int y = rect.y; y < rect.y + rect.height; y++) {
                for(s32int x = rect.x; x < rect.x + rect.width; x++) {
                    u32int indx = get_pixel(&canvas, x, y);
                    screen[indx] = media_buff[indx];
                }
            }
        }
    }
    return;
}

window_s *alertbox_create(window_s *parent, s32int x, s32int y, u8int *text, u8int *title) {
/*
    window_s *albox = window_create(parent, x, y, 200, 160, WINDOW_ALERT, "window_classic");
    window_s *ok_butt = window_create(albox, 30, 60, 110, 30, WINDOW_CONTROL, "alertbox_button");

    window_title_bar(albox);
    window_close_button(albox);

    canvas_s al_canvs = canvas_create(albox->width, albox->height, (u32int *)albox->frame_buff);
    canvas_s but_canvs = canvas_create(ok_butt->width, ok_butt->height, (u32int *)ok_butt->frame_buff);

    set_font_color(VESA_MAGENTA + 1);
    pnt_text(&al_canvs, 1, 2, title);
    pnt_text(&al_canvs, 5, 2, text);
    pnt_text(&but_canvs, 1, 1, "Close Button");

    //memcpy(vesa_mem, albox->frame_buff, albox->width * albox->height * 4);
    //memcpy(vesa_mem, ok_butt->frame_buff, ok_butt->width * ok_butt->height * 4);

    return albox;
    */
    window_s * alertbox = window_create(parent, x, y, 200, 170, WINDOW_ALERT, "window_classic");
    window_title_bar(alertbox);
    window_close_button(alertbox);
    canvas_s canvas_alertbox = canvas_create(alertbox->width, alertbox->height, alertbox->frame_buff);
    set_font_color(VESA_BLACK + 1);
    pnt_text(&canvas_alertbox, 1, 2, title);
    pnt_text(&canvas_alertbox, 5, 2, text);

    // Add a OK button for the alertbox
    window_s * ok_button = window_create(alertbox, 30, 60, 110, 30, WINDOW_CONTROL, "alertbox_button");
    canvas_s canvas_button = canvas_create(ok_button->width, ok_button->height, ok_button->frame_buff);
    pnt_text(&canvas_button, 1, 1, "Close Button");

   // memcpy(vesa_mem, alertbox->frame_buff, alertbox->width * alertbox->height * 4);
   // memcpy(vesa_mem, ok_button->frame_buff, ok_button->width * ok_button->height * 4);

    return alertbox;
}   

window_s *window_create(window_s *parent, s32int x, s32int y, s32int width, s32int height, s32int type, u8int *name) {

    gtreenode_s *subroot;

    window_s *wind = kcalloc(sizeof(window_s), 1);
    wind->frame_buff = (u32int *)kmalloc(width * height * 4);
    wind->blend_buff = (u32int *)kmalloc(width * height * 4);
    strcpy(wind->name, name);

    wind->under_windows = list_create();
    wind->above_windows = list_create();

    wind->x = x;
    wind->y = y;
    wind->width = width;
    wind->height = height;
    wind->type = type;
    wind->parent = parent;
    wind->is_minimized = 0;
    wind->depth = 0;

    if(parent) {
        if(!strcmp(name, "window_red")) {memsetdw((u32int *)wind->frame_buff, 0xFFFF0000, width * height);}
        else if(!strcmp(name, "window_green")) {memsetdw((u32int *)wind->frame_buff, 0xFF00FF00, width * height);}
        else if(!strcmp(name, "window_blue")) {memsetdw((u32int *)wind->frame_buff, 0xFF0000FF, width * height);}
        else if(!strcmp(name, "window_black")) {memsetdw((u32int *)wind->frame_buff, 0xFF300a24, width * height);}
        else if(!strcmp(name, "window_classic")) {memsetdw((u32int *)wind->frame_buff, 0xFFEEEEEE, width * height);}
        else if(!strcmp(name, "window_xp")) {memsetdw((u32int *)wind->frame_buff, 0xFFF7F3F0, width * height);}
        else if(!strcmp(name, "alertbox_button")) {memsetdw((u32int *)wind->frame_buff, 0xFFF7F3F0, width * height);}
        else if(!strcmp(name, "desktop_bar")) {memsetdw((u32int *)wind->frame_buff, 0xFFDCE8EC, width * height);}
    }

    if(type == WINDOW_SUPER) {
        subroot = NULL;
    }
    else if(type == WINDOW_NORMAL) {
        subroot = tree_windows->root;
    }
    else {
        subroot = parent->self;
    }
    wind->self = tree_insert(tree_windows, subroot, wind);

    //cur = pprint(&cnvs, cur.x, cur.y, "in wind_create():wind(%s), type(%d), self node(%x), \n", wind->name, wind->type, wind->self);
    
    recur_add_under_windows(wind, tree_windows->root);

    if(!strcmp(wind->name, "desktop_bar")) {
        desktop_bar = wind;
    }
    focus_window = wind;

    return wind;
}

void window_title_bar(window_s *wind) {

    canvas_s canvs = canvas_create(wind->width, wind->height, (u32int *)wind->frame_buff);

    for(u32int i = 0; i < TITLE_BAR_HEIGHT; i++) {
        set_fillcolor(colors_title_bar[i] | 0x88000000);
        pnt_line(&canvs, 0, i, wind->width, i);
    }
    return;
}

void window_close_button(window_s *wind) {

    canvas_s canvs = canvas_create(wind->width, wind->height, (u32int *)wind->frame_buff);
    draw_rect_region(&canvs, &close_reg);

    return;
}

void window_minimize_button(window_s *wind) {

    canvas_s cnvs = canvas_create(wind->width, wind->height, wind->frame_buff);
    draw_rect_region(&cnvs, &minimize_reg);
    return;
}

void window_maximize_button(window_s *wind) {

    canvas_s cnvs = canvas_create(wind->width, wind->height, wind->frame_buff);
    draw_rect_region(&cnvs, &maximize_reg);
    return;
}

void recur_add_under_windows(window_s *wind, gtreenode_s *subroot) {

    window_s *curr_wind = subroot->value;
//cur = pprint(&cnvs, cur.x, cur.y, "if add_und_wind():wind = %x; curr_wind = %x\n", wind, curr_wind);
    if(curr_wind == wind) {
        return;
    }

    if(is_windows_overlap(wind, curr_wind)) {

//cur = pprint(&cnvs, cur.x, cur.y, "overlap \n");

        if(list_contain(curr_wind->above_windows, wind) == -1) {
            list_insert_back(curr_wind->above_windows, wind);
            curr_wind->depth = list_size(curr_wind->above_windows);
        }
        if(list_contain(wind->under_windows, curr_wind) == -1) {
            list_insert_back(wind->under_windows, curr_wind);
        }
    }


    if(subroot->children->first == NULL) {
        goto e;
    }
    list_item_s *child = subroot->children->first;

    do {
        recur_add_under_windows(wind, child->data);
        child = child->next;

    } while(child != subroot->children->first);
e:
    return;
}

void window_display(window_s *wind, rect_s *rects, s32int size) {

    region_rect_s rect_region;

    if(wind->is_minimized || wind->parent->is_minimized) {
        return;
    }
    set_fillcolor(0xFFFFFF00);

    if(wind->frame_buff) {

        rect_region.region = (u32int *)wind->blend_buff;
        point_s point = get_dev_coordns(wind);

        if(rects) {
            for(s32int i = 0; i < size; i++) {
                rect_region.rect = rects[i];
                paint2_clip_rect(&cnvs, &rect_region, wind->width, point.x, point.y); //!!!!!!!!!!! cnvs??
            }
        }
        else {
            rect_region.rect.x = point.x;
            rect_region.rect.y = point.y;
            rect_region.rect.width = wind->width;
            rect_region.rect.height = wind->height;

            memsetdw(rect_region.region, 0x00FFFF00, wind->width * wind->height * 4);

            draw_rect_region(&cnvs, &rect_region);      //!!!!!!!!!!!!!!!  cnvs??
        }
        display_recur(wind->self);
    }
    return;
}

void display_recur(gtreenode_s *parent) {

    if(parent->children == NULL) {
        goto ex;
    }
    list_item_s *child = parent->children->first;

    do{

        gtreenode_s *node = child->data;
        window_s *wind = node->value;

        if(!node || !wind) {
            return;
        }

        if(wind->type == WINDOW_CONTROL) {
            window_display(wind, NULL, 0);
        }
        child = child->next;

    } while(child != parent->children->first);

    ex:
    return;
}

point_s get_dev_coordns(window_s *wind) {

    point_s point;
    window_s *parent = wind->parent;

    point.x = wind->x;
    point.y = wind->y;

    while(parent != NULL) {

        point.x += parent->x;
        point.y += parent->y;
        parent = parent->parent;
    }
    return point;
}

point_s get_dev_relative_coordns(window_s *wind, s32int x, s32int y) {

    point_s point = get_dev_coordns(wind);
    point.x = x - point.x;
    point.y = y - point.y;

    return point;
}

s32int is_windows_overlap(window_s *w1, window_s *w2) {

    point_s point1 = get_dev_coordns(w1);
    point_s point2 = get_dev_coordns(w2);

    rect_s r1 = rect_create(point1.x, point1.y, w1->width, w2->height);
    rect_s r2 = rect_create(point2.x, point2.y, w2->width, w2->height);

    s32int ret = is_rect_overlap(r1, r2);

    return ret;
}

void intersections_calc(rect_s *rect, window_s **array, u32int *ret_size) {

    u32int size = 0;
    u32int new_size = 0;
    rect_s cur_rect;
    point_s p;

    tree2array(tree_windows->root, (void **)array, &size);

    for(u32int i = 0; i < size; i++) {
        window_s *c_w = array[i];

        if(c_w->is_minimized) {
            continue;
        }
        p = get_dev_coordns(c_w);
        cur_rect = rect_create(p.x, p.y, c_w->width, c_w->height);

        if(is_rect_overlap(*rect,  cur_rect)) {
            c_w->intersect_rect = find_rect_overlab(*rect, cur_rect);
            array[new_size++] = c_w;
        }
    }
    *ret_size = new_size;

    return;
}

void sort_wind_array(window_s **array, u32int size) {

    window_s *tmp;

    for(u32int i = 0; i < size - 1; i++) {
        for(u32int j = 0; j < size - 1; j++) {

            if(array[j]->depth < array[j+1]->depth) {
                tmp = array[j];
                array[j] = array[j+1];
                array[j+1] = tmp;
            }
        }   
    }
    return;
}

u32int get_wind_pixel(window_s *wind, s32int x, s32int y) {

    point_s p = get_dev_relative_coordns(wind, x, y);
    s32int indx = wind->width * p.y + p.x;
    u32int ret = wind->frame_buff[indx];

    return ret;
}

void blend_wind_rect(window_s *top_w, window_s *und_w) {

    u32int top_col;
    u32int und_col;
    u32int blen_col;
    u32int indx;
    point_s p;

    rect_s *und_rect = &und_w->intersect_rect;

    for(u32int x = und_rect->x; x < und_rect->x + und_rect->width; x++) {
        for(u32int y = und_rect->y; y < und_rect->y + und_rect->height; y++) {

            top_col = get_wind_pixel(top_w, x, y);
            p = get_dev_relative_coordns(und_w, x, y);
            indx = und_w->width * p.y + p.x;
            und_col = und_w->blend_buff[indx];

            //blen_col = blend_colors(top_col, und_col);          //!!!FPU MATH EXEPTION
            p = get_dev_relative_coordns(top_w, x, y);
            indx = top_w->width * p.y + p.x;
            top_w->blend_buff[indx] = blen_col;
        }
    }
    return;
}

void blend_windows(window_s *wind) {

    if(wind->type == WINDOW_SUPER) {
        memcpy(wind->blend_buff, wind->frame_buff, wind->width * wind->height * 4);
        return;
    }
    point_s p = get_dev_coordns(wind);
    s32int xold = p.x;
    s32int yold = p.y;
    s32int wiold = wind->width;
    s32int heold = wind->height;
    u32int size;

    rect_s rect = rect_create(xold, yold, wiold, heold);
    intersections_calc(&rect, windows_array, &size);
    sort_wind_array(windows_array, size);

    for(u32int i = 0; i < size; i++) {

        if(windows_array[i]->depth > wind->depth) {
            blend_wind_rect(wind, windows_array[i]);
        }
    }
    return;
}

window_s *get_super_window(void) {
    return tree_windows->root->value;
}

void window_round_corner(window_s *wind) {

    canvas_s cnvs = canvas_create(wind->width, wind->height, wind->frame_buff);
    round_corner_effect(&cnvs);
    return;
}

void wind_test(void) {

    cur.x = 10;
    cur.y = 10;

    close_reg.rect.x = 7;
    close_reg.rect.y = 3;
    close_reg.rect.width = 10;
    close_reg.rect.height = 5;

    u32int *frame_buff = kmalloc((1280 + 720) * 4);
    cnvs = canvas_create(1280, 720, vesa_mem);
    memsetdw(frame_buff, VESA_UBUNT_TERM, cnvs.width * cnvs.height);
    memcpy(vesa_mem, frame_buff, cnvs.height * cnvs.width * 4);

    tree_windows = tree_create();

    window_s *s = window_create(NULL, 0, 0, 1280, 720, WINDOW_SUPER, "desktop");

    blend_windows(s);
    window_display(s, NULL, 0);
    memcpy(vesa_mem, s->blend_buff, s->width * s->height * 4);

    window_s *al = alertbox_create(s, 50, 100, "bbbbb", "xp");
    blend_windows(al);
    window_display(al, NULL, 0);
    memcpy(vesa_mem, al->blend_buff, al->width * al->height * 4);

    pprint(&cnvs, cur.x, cur.y, "fajsfiweofoiweuiweigjfiwjijwegoihrughokjjsoifjweoifhweohfwie\nwioeoiweu/nwfiofi");

    //window_s *box1 = window_create(s, 310, 200, 400, 300, WINDOW_NORMAL, "window_xp");
    //window_s *box2 = window_create(s, 310, 100, 200, 200, WINDOW_NORMAL, "window_xp");
/*
    window_display(box1, NULL, 0);
    blend_windows(box1);
    memcpy(vesa_mem, box1->blend_buff, box1->width * box1->height * 4);

    window_display(box2, NULL, 0);
    blend_windows(box2);
    memcpy(vesa_mem, box2->blend_buff, box2->width * box2->height * 4);
*/
    return;
}

void gui_init(void) {

    vesa_init();
    windows_init();

    //terminal init;
    window_s *term = window_create(get_super_window(), 40, 200, 750, 450, WINDOW_NORMAL, "window_black");
    window_title_bar(term);
    window_close_button(term);
    window_minimize_button(term);
    window_maximize_button(term);

    canvas_s cnvs_term = canvas_create(term->width, term->height, term->frame_buff);
    set_font_color(VESA_BLACK + 1);
    pnt_text(&cnvs_term, 1, 2, "@TERMINAL");
    window_round_corner(term);
    blend_windows(term);

    //Desktop bar init (top);
    window_s *bar = window_create(get_super_window(), 0, 0, screen_width, screen_height / 30, WINDOW_DESKT_BAR, "desktop_bar");
    canvas_s cnvs_bar = canvas_create(bar->width, bar->height, bar->frame_buff);
    set_font_color(VESA_BLACK + 1);
    pnt_text(&cnvs_bar, 1, 115, "22:09\n13.03.2023");
   // blend_windows(bar);     //->blend_wind_rect->blend_colors  FPU exeption !!! need correct
    //display_all_windows();
    media_mem_upd(NULL, 0);

    return;
}