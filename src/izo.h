#ifndef     IZO_H
#define     IZO_H

#include    "common.h"
#include    "help_func.h"
#include    "vesa.h"
#include    "font.c"
//#include    "windows.h"

typedef struct canvas canvas_s;
typedef struct region_rect region_rect_s;
typedef struct rect rect_s;
typedef struct cursor cursor_s;

extern void *vesa_mem;

struct canvas {

    s32int  width;
    s32int  height;
    u32int  *buff;
};

struct rect {

    s32int  x;
    s32int  y;
    s32int  width;
    s32int  height;
};

struct region_rect {

    rect_s rect;
    u32int *region;
};

struct cursor {

    s32int x;
    s32int y;
};

#define SET_ALPHA(color, alpha)     (((color << 8) | >> 8) | ((alpha << 24) & 0xFF000000))
#define abs(x)                      ( (x < 0) ? -(x): x )
#define sign(x)                     ((x < 0) ? -1: ((x > 0) ? 1: 0))
#define min(x, y)                   ( (x < y) ? x : y )
#define max(x, y)                   ( (x > y) ? x : y )

#define FONT_HEIGHT                 0x08
#define FONT_WIDTH                  0x08


#define GET_ALPHA(color) ((color >> 24) & 0x000000FF)
#define GET_RED(color)   ((color >> 16)   & 0x000000FF)
#define GET_GREEN(color) ((color >> 8)  & 0x000000FF)
#define GET_BLUE(color)  ((color >> 0)   & 0X000000FF)

canvas_s canvas_create(s32int width, s32int height, u32int *buff);
rect_s rect_create(s32int x, s32int y, s32int width, s32int height);
region_rect_s region_create(rect_s rect, u32int *region);

s32int get_pixel(canvas_s *canvas, s32int x, s32int y);
void set_pixel(canvas_s *canvas, u32int val, s32int x, s32int y);
void set_fillcolor(u32int color);
void draw_rect(canvas_s *canvas, s32int x, s32int y, s32int width, s32int height);
void draw_rect_region(canvas_s *canvas, region_rect_s *region);
void paint_clip_rect(canvas_s *canvas, region_rect_s *region, s32int crop_width);
void paint2_clip_rect(canvas_s *canvas, region_rect_s *region, s32int crop_width, s32int crop_x, s32int crop_y);

void round_corner_effect(canvas_s *canvas);
void remov_sharp_edges(canvas_s *canvas, s32int st_x, s32int st_y, s32int direct, s32int num_pixels, u32int alpha_end, u32int alpha_middle);

void pnt_line(canvas_s *canvs, s32int x0, s32int y0, s32int x1, s32int y1);
cursor_s pnt_text(canvas_s *canvs, s32int st_x, s32int st_y, u8int *text);
cursor_s pnt_hex(canvas_s *canvs, s32int x, s32int y, u32int val);
cursor_s pnt_dec(canvas_s *canvs, s32int x, s32int y, u32int val);
cursor_s pprint(canvas_s *canvs, s32int x, s32int y, u8int *text, ...);

s32int is_line_overlap(s32int line1_x1, s32int line1_x2, s32int line2_x1, s32int line2_x2);
s32int is_rect_overlap(rect_s rect0, rect_s rect1);
s32int is_point_in_rect(s32int x, s32int y, rect_s *rect);
rect_s find_rect_overlab(rect_s rect0, rect_s rect1);

u32int blend_colors(u32int color1, u32int color2);
u32int get_font_color(void);
void set_font_color(u32int color);

#endif